#include "multithread_version.h"

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

TCPSTATE MultiAcceptor::get_state() { return MultiAcceptorImp::get_state(); }

MultiServer::MultiServer(size_t idle, Ipv4Addr listen_ip) 
    : is_accepting_(false), 
    idle_duration_(idle),
    listen_ip_(listen_ip) {

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
    std::unique_lock<mutex> lk(syncmutex_);
    is_accepting_ = true;
    EventManagerWrapper emw;
    MultiAcceptor x_acceptor(&emw);
    x_acceptor
        .set_listen_addr(listen_ip_)
        .set_accept_readable_callback(
            [this](pair<FileDescriptorType, Ipv4Addr> p) { p_queue_->push_one(move(p)); })
        .epoll_and_accept();
    lk.unlock();
    synccv_.notify_all();
    emw.loop();
}

void MultiServer::MsgServerRoutineDetail() {
    std::unique_lock<mutex> lk(syncmutex_);
    synccv_.wait(lk, [this](){ return is_accepting_ == true; });
    EventManagerWrapper emw;
    size_t seqno = 0;
    map<size_t, shared_ptr<TCPConnection>> tcpcon_map;
    std::function<void()> fetch_routine = [this, &tcpcon_map, &emw, &fetch_routine](){
        vector<FileDescriptorType, Ipv4Addr> newpairs;
        if (FetchOne(newpairs)) {
            for (auto newpair : newpairs) {
                // generate tcp con and set epoll
                shared_ptr<TCPConnection> shared_p_tc( 
                    new TCPConnection(emw, get<0>(newpair), listen_ip_, get<1>(newpair)));
                seqno++;
                tcpcon_map[seqno] = shared_p_tc;
                shared_p_tc->set_seqno_of_server(seqno);
                shared_p_tc->set_normal_readable_callback([this, seqno - 1](TCPConnection& this_con) {
                    auto check = this_con.to_read();
                    if (check >= 1) {
                        if (msg_responser_cb_ != nullptr && msg_cb_ == nullptr) {
                            auto& rbuffer = this_con.get_rb_ref();
                            auto& wbuffer = this_con.get_wb_ref();
                            msg_responser_cb_(seqno_tmp, rbuffer, wbuffer,
                                              this_con.get_bigfilesendcb());
                            auto check1 = this_con.to_write();
                            LOG_INFO("server respond %d bytes", check1);
                        } else if (msg_cb_ != nullptr &&
                                   msg_responser_cb_ == nullptr) {
                            msg_cb_(seqno_tmp);
                            auto check1 = this_con.to_write();
                            LOG_INFO("server respond %d bytes", check1);
                        } else {
                            ABORT(
                                    "did you remember to set callback, or duplicate or "
                                            "conflict?");
                        }
                    } else {
                        if (this_con.get_state() == TCPSTATE::Peerclosed) {
                            LOG_WARN("no bytes to read in read cb, if it's level triggered, it means peer close, we gonna to local_close this");       // if you use level triggered, it's the problem
                            this_con.local_close();
                        } else if (this_con.get_state() == TCPSTATE::Localclosed){
                            NOTDONE();
                        } else {
                            this_con.local_close();
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
        emw.run_after(idle_duration, fetch_routine);
    };
    emw.run_after(0, fetch_routine);
}

void MultiServer::MsgServerRoutine() {
    unique_lock<mutex> lk(syncmutex_);
    synccv_.wait(lk, [](){
                if (true) {
                    return true;
                }
            });
}

bool MultiServer::FetchOne(vector<FileDescriptorType>& toit) {
    auto ret = p_queue_->get_one();
    if (ret.empty()) {
        return false;
    } else {
        toit = move(ret);
        return true;
    }
}

MultiServer& MultiServer::set_msg_callback(MsgCallBack&& cb) {
    msg_cb_ = std::move(cb);
    return *this;
}

MultiServer& MultiServer::set_msg_responser_callback(MsgResponserCallBack&& cb) {
    msg_responser_cb_ = std::move(cb);
    return *this;
}

} /* my_http  */
