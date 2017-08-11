#include "multithread_version.h"
#include "event.h"

namespace my_http {

    detail::MultiAcceptorImp::MultiAcceptorImp(EventManagerWrapper* emwp) : Acceptor(emwp) {

    }

    detail::MultiAcceptorImp::~MultiAcceptorImp() {}

void detail::MultiAcceptorImp::handle_epoll_readable() {
    // when socketfd is readable, you can accept new fd
    int new_fd = -1;
    Ipv4Addr peer;
    unsigned int len = sizeof(*peer.get_p_socketaddr());
    while (listened_socket_->get_fd() &&
           (new_fd = ::accept(listened_socket_->get_fd(),
                              peer.get_p_socketaddr(), &len)) >= 0) {
        // accept all socket, then put them to others, by cb_
        if (new_fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // no more
            break;
        } else if (new_fd < 0) {
            LOG_ERROR("errno = %s", strerror(errno));
            break;
        }
        auto check = ::getpeername(new_fd, peer.get_p_socketaddr(), &len);
        if (check != 0) {
            SLOG_ERROR("errno = " << strerror(errno));
            NOTDONE();
        }
        set_nonblock(new_fd);
        movecb_x_(make_pair(new_fd, peer));
    }
}

detail::MultiAcceptorImp&
detail::MultiAcceptorImp::set_accept_readable_callback(MoveFDCallBack&& cb) {
    movecb_x_ = move(cb);
    return *this;
}

    void detail::MultiAcceptorImp::listen_it_and_accept() {
        listened_ip_.ip_bind_socketfd(listened_socket_->get_fd());
        auto check = ::listen(listened_socket_->get_fd(), SOMAXCONN);
        if (check != 0) {
            NOTDONE();
        }
        tcpstate_ = TCPSTATE::Listening;
        if (movecb_x_ != nullptr) {
            LOG_INFO("acceptor register accept epoll readable cb");
            emp_->register_event(
                    Channel::get_readonly_event_flag(), listened_socket_.get(),
                    [this]() {
                        SLOG_INFO("in Acceptor::accept callback, accepted_count_ ="
                                          << accepted_count_++);
                        handle_epoll_readable();
                    });
        } else {
            NOTDONE();
        }
    }

    void detail::MultiAcceptorImp::epoll_and_accept(time_ms_t after) {
        emp_->run_after(after, std::bind(&MultiAcceptorImp::listen_it_and_accept, this));
    }

MultiAcceptor::MultiAcceptor(EventManagerWrapper* emwp) : MultiAcceptorImp(emwp) {

}

MultiAcceptor::~MultiAcceptor() {

}

MultiAcceptor& MultiAcceptor::set_listen_addr(Ipv4Addr addr) {
    MultiAcceptorImp::set_listen_addr(std::move(addr));
    return *this;
}

MultiAcceptor& MultiAcceptor::set_accept_readable_callback(
    MoveFDCallBack&& cb) {
    MultiAcceptorImp::set_accept_readable_callback(move(cb));
    return *this;
}

void MultiAcceptor::epoll_and_accept(time_ms_t after) {
    MultiAcceptorImp::epoll_and_accept(after);
}

//TCPSTATE MultiAcceptor::get_state() { return MultiAcceporImp::get_state(); }

MultiServer::MultiServer(size_t idle, Ipv4Addr listen_ip) 
    : is_accepting_(false), 
    idle_duration_(idle),
    listen_ip_(listen_ip),
p_queue_(new AcceptedQueue<Qtype>()){
        default_current_thread_block_check_ = [this](){
            return threads_in_pool_ends_;
        };
    }

MultiServer::~MultiServer() {}

void MultiServer::AcceptorRoutine() {
    if (is_accepting_ == false) {
        AcceptorRoutineDetail();
    } else {
        NOTDONE();
    }
}

void MultiServer::AcceptorRoutineDetail() {
    EventManagerWrapper emw;
    MultiAcceptor x_acceptor(&emw);
    x_acceptor
        .set_listen_addr(listen_ip_)
        .set_accept_readable_callback(
            [this](pair<FileDescriptorType, Ipv4Addr>&& p) { p_queue_->push_one(move(p)); })
        .epoll_and_accept();
    std::unique_lock<mutex> lk(syncmutex_);
    is_accepting_ = true;
    pool_stop_cb_vec_.emplace_back([&emw](){
        emw.exit();
    });
    lk.unlock();
    synccv_.notify_all();
    emw.loop();
}

void MultiServer::MsgServerRoutineDetail() {
    EventManagerWrapper emw;
    size_t seqno = 0;
    map<size_t, shared_ptr<TCPConnection>> tcpcon_map;
    auto fetch_routine = get_fetch_routine(tcpcon_map, emw, seqno);
    emw.run_after(100, move(fetch_routine), idle_duration_);
    LOG_DEBUG("before routine ");
    std::unique_lock<mutex> lkk(syncmutex_);
    pool_stop_cb_vec_.emplace_back([&emw](){
        emw.exit();
    });
    lkk.unlock();
    emw.loop();
}

    //! @brief a routine fetch accepted fd and set epoll at local eventmanager
    std::function<void()> MultiServer::get_fetch_routine(map<size_t, shared_ptr<TCPConnection>>& tcpcon_map, EventManagerWrapper& emw, size_t& seqno) {
        return [this, &tcpcon_map, &emw, &seqno](){
            vector<pair<FileDescriptorType, Ipv4Addr>> newpairs;
            if (FetchOne(newpairs)) {
                for (auto newpair : newpairs) {
                    LOG_DEBUG("fetch one");
                    // generate tcp con and set epoll
                    auto fd = get<0>(newpair);
                    auto ip = get<1>(newpair);
                    shared_ptr<TCPConnection> shared_p_tc(
                            new TCPConnection(emw.get_pimpl(), fd, listen_ip_, ip));
                    seqno++;
                    tcpcon_map[seqno] = shared_p_tc;
                    shared_p_tc->set_seqno_of_server(seqno);
                    shared_p_tc->set_normal_readable_callback([this, seqno](TCPConnection& this_con) {
                                LOG_DEBUG("one readable callback");
                                auto check = this_con.to_read();
                                if (check >= 1) {
                                    if (msg_responser_cb_ != nullptr && tcp_cb_ == nullptr) {
                                        auto& rbuffer = this_con.get_rb_ref();
                                        auto& wbuffer = this_con.get_wb_ref();
                                        msg_responser_cb_(seqno, rbuffer, wbuffer,
                                                          this_con.get_bigfilesendcb());
                                        auto check1 = this_con.to_write();
                                        LOG_INFO("server respond %d bytes", check1);
                                    } else if (tcp_cb_ != nullptr &&
                                               msg_responser_cb_ == nullptr) {
                                        tcp_cb_(this_con); // user can't get this_con through this
                                        auto check1 = this_con.to_write();
                                        LOG_INFO("server respond %d bytes", check1);
                                    } else {
                                        ABORT( "did you remember to set callback, or duplicate or conflict?");
                                    }
                                    this_con.do_lazy_close();
                                    //if (after_to_write_cb_ != nullptr) {
                                        //after_to_write_cb_(this_con);
                                    //}
                                } else {
                                    if (this_con.get_state() == TCPSTATE::Peerclosed) {
                                        LOG_WARN("no bytes to read in read cb, if it's level triggered, it means peer close, we gonna to local_close this");       // if you use level triggered, it's the problem
                                        this_con.local_close();
                                    } else if (this_con.get_state() == TCPSTATE::Localclosed){
                                        NOTDONE();
                                    } else {
                                        LOG_ERROR("???");
                                        //this_con.local_close();
                                    }
                                }
                            })
                            .set_local_close_callback([this, &tcpcon_map](TCPConnection& this_con) {
                                LOG_DEBUG("server : local close cb");
                                //remove_tcpcon_by_seqno(this_con.get_seqno());
                                auto found = tcpcon_map.find(this_con.get_seqno());
                                if (found != tcpcon_map.end()) {
                                    tcpcon_map.erase(found);
                                }
                            })
                            .set_peer_close_callback([this](TCPConnection& this_con) {
                                LOG_DEBUG("server : peer down cb");
                            })
                            .epoll_and_conmunicate();
                }
            }
        };
    }

void MultiServer::MsgServerRoutine() {
    std::unique_lock<mutex> lk(syncmutex_);
    synccv_.wait(lk, [this](){ if (is_accepting_ == true) { return true; } });
    lk.unlock();
    MsgServerRoutineDetail();
}

bool MultiServer::FetchOne(vector<pair<FileDescriptorType, Ipv4Addr>>& toit) {
    auto ret = p_queue_->get_one();
    if (ret.empty()) {
        return false;
    } else {
        toit = move(ret);
        return true;
    }
}

MultiServer& MultiServer::set_tcp_callback(TCPCallBack && cb) {
    tcp_cb_ = std::move(cb);
    return *this;
}

MultiServer& MultiServer::set_msg_responser_callback(MsgResponserCallBack&& cb) {
    msg_responser_cb_ = std::move(cb);
    return *this;
}

void MultiServer::ThreadPoolStart(function<bool()>&& block_checker) {
    if (block_checker != nullptr) {
        default_current_thread_block_check_ = block_checker;
    }
    if (ThreadPool::get_default_threadpool_size() > 2) {
        //auto amount = ThreadPool::get_default_threadpool_size();
        int amount = 2;
        thread_pool_.add_task(std::bind(&MultiServer::AcceptorRoutine, this));
        while (--amount > 0) {
            thread_pool_.add_task(std::bind(&MultiServer::MsgServerRoutine, this));
        }
        pool_stop_cb_ = [this](){
            for (auto cb : pool_stop_cb_vec_) {
                cb();
            }
        };
        LOG_DEBUG("before thread_pool start");
        thread_pool_.start();   //!< threads would block at loop() but this main thread won't block
        {
            std::unique_lock<mutex> lk(cur_block_mutex_);
            cur_block_cv_.wait(lk, default_current_thread_block_check_);
        }
    } else {
        NOTDONE();
    }
}

    //! @brief this function can be cast to other threads
void MultiServer::Exit() {
    pool_stop_cb_();
    thread_pool_.stop_w();
    threads_in_pool_ends_ = true;
}

    //MultiServer& MultiServer::set_after_to_write_cb(TCPCallBack &&cb) {
    //    after_to_write_cb_ = move(cb);
    //    return *this;
    //}

} /* my_http  */
