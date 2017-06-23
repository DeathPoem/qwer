#include "tcpserver.h"

namespace my_http {

std::ostream& operator<<(std::ostream& os, TCPSTATE state) {
    string str;
    if (state == TCPSTATE::Invalid) {
        str = "Invalid";
    } else if (state == TCPSTATE::Listening) {
        str = "Listening";
    } else if (state == TCPSTATE::Afterhandshake) {
        str = "Afterhandshake";
    } else if (state == TCPSTATE::Tryconnect) {
        str = "Tryconnect";
    } else if (state == TCPSTATE::Gooddead) {
        str = "Gooddead";
    } else if (state == TCPSTATE::Peerclosed) {
        str = "Peerclosed";
    } else if (state == TCPSTATE::Localclosed) {
        str = "Localclosed";
    } else if (state == TCPSTATE::Failed) {
        str = "Failed";
    } else if (state == TCPSTATE::Newborn) {
        str = "Newborn";
    } else if (state == TCPSTATE::Connected) {
        str = "Connected";
    } else {
        NOTDONE();
    }
    os << str;
    return os;
}

Acceptor::Acceptor(EventManagerWrapper* emwp)
    : emp_(emwp->get_pimpl()),
      listened_socket_(new Channel(detail::create_socketfd())) {
    int listen_fd = listened_socket_->get_fd();
    if (listen_fd < 0) {
        NOTDONE();
    }
    set_nonblock(listen_fd);
    tcpstate_ = TCPSTATE::Newborn;
}

Acceptor::~Acceptor() {
    // TODO
    // disregister
    SLOG_INFO("Acceptor destruct"
              << ";tcp state=" << tcpstate_);
}

Acceptor& Acceptor::set_listen_addr(Ipv4Addr addr) {
    // std::swap(listened_ip_, addr);
    listened_ip_ = addr;
    return *this;
}

Acceptor& Acceptor::set_accept_readable_callback(
    MoveTCPConnectionCallBack&& cb) {
    movecb_ = std::move(cb);
    return *this;
}

void Acceptor::listen_it_and_accept() {
    listened_ip_.ip_bind_socketfd(listened_socket_->get_fd());
    auto check = ::listen(listened_socket_->get_fd(), SOMAXCONN);
    if (check != 0) {
        NOTDONE();
    }
    tcpstate_ = TCPSTATE::Listening;
    if (movecb_ != nullptr) {
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

void Acceptor::handle_epoll_readable() {
    // when socketfd is readable, you can accept new fd
    int new_fd = -1;
    Ipv4Addr peer;
    unsigned int len = sizeof(*peer.get_p_socketaddr());
    while (listened_socket_->get_fd() &&
           (new_fd = ::accept(listened_socket_->get_fd(),
                              peer.get_p_socketaddr(), &len)) >= 0) {
        // accept all socket, then put them to others, by cb_
        if (new_fd < 0) {
            // no more
            break;
        }
        auto check = ::getpeername(new_fd, peer.get_p_socketaddr(), &len);
        if (check != 0) {
            SLOG_ERROR("errno = " << strerror(errno));
            NOTDONE();
        }
        set_nonblock(new_fd);
        shared_ptr<TCPConnection> shared_p_tc(
            new TCPConnection(emp_, new_fd, listened_ip_, peer));
        movecb_(std::move(shared_p_tc));
    }
}

void Acceptor::epoll_and_accept(time_ms_t after) {
    LOG_INFO("in epoll_and_accept");
    emp_->run_after(after, std::bind(&Acceptor::listen_it_and_accept, this));
}

int detail::create_socketfd() {
    int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
    return socketfd;
}

Connector::Connector(EventManagerWrapper* emwp)
    : emp_(emwp->get_pimpl()),
      to_connect_socket_(new Channel(detail::create_socketfd())) {
    int fd = to_connect_socket_->get_fd();
    if (fd < 0) {
        NOTDONE();
    }
    set_nonblock(fd);
    tcpstate_ = TCPSTATE::Newborn;
}

Connector::~Connector() {
    // TODO
    // disregister
    LOG_INFO("Acceptor destruct");
}

Connector& Connector::set_local_addr(Ipv4Addr addr) {
    // std::swap(local_ip_, addr);
    local_ip_ = addr;
    return *this;
}

Connector& Connector::set_connect_to_addr(Ipv4Addr addr) {
    // std::swap(to_connect_ip_, addr);
    to_connect_ip_ = addr;
    return *this;
}

Connector& Connector::set_connect_callback(MoveTCPConnectionCallBack&& cb) {
    movecb_ = std::move(cb);
    return *this;
}

void Connector::connect_it() {
    tcpstate_ = TCPSTATE::Tryconnect;
    if (connect_stretegy_flag_ == 0) {
        try_connect_once();
        if (tcpstate_ == TCPSTATE::Connected) {
            shared_ptr<TCPConnection> shared_p_tc(
                new TCPConnection(emp_, std::move(to_connect_socket_),
                                  local_ip_, to_connect_ip_));
            movecb_(std::move(shared_p_tc));
            close();
        } else {
            if (retry_count_++ < 5) {
                emp_->run_after(100, std::bind(&Connector::connect_it, this));
            } else {
                LOG_WARN("retry connect fail");
            }
        }
    } else {
        NOTDONE();
    }
}

void Connector::close() {
    if (to_connect_socket_ == nullptr) {
        tcpstate_ = TCPSTATE::Gooddead;
    } else {
        tcpstate_ = TCPSTATE::Failed;
    }
}

void Connector::handle_epoll_connected() {
    LOG_INFO("in connector-handle epoll connected()");
    local_ip_.ip_bind_socketfd(to_connect_socket_->get_fd());
    connect_it();
}

Connector& Connector::set_connect_stretegy(int flag) {
    connect_stretegy_flag_ = flag;
    return *this;
}

void Connector::try_connect_once() {
    unsigned int len = sizeof(*to_connect_ip_.get_p_socketaddr());
    int check = ::connect(to_connect_socket_->get_fd(),
                          to_connect_ip_.get_p_socketaddr(), len);
    if (check != 0 && errno == 115) {
        SLOG_WARN("when ::connect to nonblocking socket fd, would cause this"
                  << strerror(errno) << ", the socket would be ready in short");
        tcpstate_ = TCPSTATE::Connected;
    } else if (check != 0 && errno != 115) {
        SLOG_WARN("fail to connect, try to connect more"
                  << ";errorstr is " << strerror(errno) << ";errno=" << errno);
    } else if (check == 0) {
        tcpstate_ = TCPSTATE::Connected;
    } else {
        NOTDONE();
    }
}

void Connector::epoll_and_connect(time_ms_t after) {
    LOG_INFO("in connector epoll and connect");
    emp_->run_after(after, std::bind(&Connector::handle_epoll_connected, this));
}

TCPConnection::TCPConnection(EventManager* emp, int fd, Ipv4Addr local,
                             Ipv4Addr peer)
    : emp_(emp), local_(local), peer_(peer), unique_p_ch_(new Channel(fd)) {
    tcpstate_ = TCPSTATE::Afterhandshake;
}

TCPConnection::TCPConnection(EventManager* emp, unique_ptr<Channel> socket_ch,
                             Ipv4Addr local, Ipv4Addr peer)
    : emp_(emp),
      local_(local),
      peer_(peer),
      unique_p_ch_(std::move(socket_ch)) {
    tcpstate_ = TCPSTATE::Afterhandshake;
}

TCPConnection::~TCPConnection() {
    // TODO
    // disregister
    if (tcpstate_ == TCPSTATE::Localclosed || tcpstate_ == TCPSTATE::Peerclosed) {
        LOG_INFO("one TCPConnection normal destruct");
    }
}

TCPConnection& TCPConnection::set_normal_readable_callback(TCPCallBack&& cb) {
    nread_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_normal_writable_callback(TCPCallBack&& cb) {
    nwrite_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_peer_close_callback(TCPCallBack&& cb) {
    close_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_local_close_callback(TCPCallBack&& cb) {
    local_close_cb_ = std::move(cb);
    return *this;
}

TCPSTATE TCPConnection::get_state() { return tcpstate_; }

BigFileSendCallBack TCPConnection::get_bigfilesendcb() {
    return [](string s) { LOG_ERROR("not done yet"); };
}

TCPConnection& TCPConnection::set_idle_callback() { return *this; }

void TCPConnection::epoll_and_conmunicate() {
    if (nread_cb_ != nullptr) {
        // emp_->register_event(Channel::get_readonly_event_flag() |
        // Channel::get_edge_trigger_flag(), unique_p_ch_.get(),
        // FIXME this code won't detect socket read? the following code would lead to empty read socket
        // FIXME when we use it as edge triggered mode, error
        emp_->register_event(Channel::get_readonly_event_flag(),
                             unique_p_ch_.get(),
                             [this]() { handle_epoll_readable(); });
    }
    if (nwrite_cb_ != nullptr) {
        emp_->register_event(Channel::get_writeonly_event_flag(),
                             unique_p_ch_.get(),
                             [this]() { handle_epoll_writable(); });
    }
    if (close_cb_ != nullptr) {
        emp_->register_event(Channel::get_peer_shutdown_flag(),
                             unique_p_ch_.get(),
                             [this]() { handle_epoll_peer_shut_down(); });
    }
}

void TCPConnection::handle_epoll_readable() { nread_cb_(*this); }

void TCPConnection::handle_epoll_peer_shut_down() { peer_close(); }

void TCPConnection::handle_epoll_writable() { nwrite_cb_(*this); }

Buffer& TCPConnection::get_rb_ref() { return read_sock_to_this_; }

Buffer& TCPConnection::get_wb_ref() { return write_sock_from_this_; }

Ipv4Addr TCPConnection::get_peer() { return peer_; }

size_t TCPConnection::try_to_read(size_t len) {
    size_t nread;
    nread = read_sock_to_this_.write_to_buffer(unique_p_ch_->get_fd(), len);
    if (nread == len) {
        return nread;
    } else {
        SLOG_ERROR("nread = " << nread << ";len=" << len);
        NOTDONE();
    }
}

size_t TCPConnection::try_to_read() {
    int len;
    int check = ::ioctl(unique_p_ch_->get_fd(), FIONREAD, &len);
    if (check != 0) {
        NOTDONE();
    }
    if (len == 0) {
        LOG_WARN("try to read empty");
        return 0;
    } else {
        return try_to_read(len);
    }
}

string TCPConnection::read_by_string() {
    char str_c[1000];
    int nread = try_to_read();
    assert(1000 > nread);
    int check = read_sock_to_this_.read_from_buffer(str_c, nread);
    if (nread == check) {
        SLOG_INFO("check =" << check);
        // read_sock_to_this_.consume(nread);        this consume should be done
        // in app
        return string(str_c);
    } else {
        NOTDONE();
    }
}

void TCPConnection::write_by_string(string str) {
    char* str_c = &str[0];
    size_t size = str.size();
    int nwrite = write_sock_from_this_.write_to_buffer(str_c, size);
    assert(nwrite == size);
    auto check = try_to_write();
    if (check == nwrite) {
    } else {
        NOTDONE();
    }
}

size_t TCPConnection::try_to_write() {
    int len = write_sock_from_this_.get_readable_bytes();
    auto check = try_to_write(len);
    if (check == 0 || len != check) {
        LOG_WARN("bad tcpcon try to write, should write bytes = %d, real write bytes = %d", len, check);
    }
    return check;
}

size_t TCPConnection::try_to_write(size_t len) {
    int nwrite =
        write_sock_from_this_.read_from_buffer(unique_p_ch_->get_fd(), len);
    if (nwrite == len) {
        write_sock_from_this_.consume(len);
        return nwrite;
    } else {
        NOTDONE();
    }
}

TCPConnection& TCPConnection::set_seqno_of_server(uint32_t seqno) {
    seqno_ = seqno;
    return *this;
}

uint32_t TCPConnection::get_seqno() { return seqno_; }

void TCPConnection::local_close() {
    tcpstate_ = TCPSTATE::Localclosed;
    unique_p_ch_->shutdown();
    local_close_cb_(*this);
    LOG_INFO("local TCPConnection close");
}

void TCPConnection::peer_close() {
    tcpstate_ = TCPSTATE::Peerclosed;
    close_cb_(*this);
    LOG_INFO("peer close");
}

TCPServer::TCPServer(EventManagerWrapper* emwp, Ipv4Addr listen_ip,
                     uint32_t maxtcpcon)
    : emp_(emwp->get_pimpl()),
      listen_ip_(listen_ip),
      maxtcpcon_(maxtcpcon),
      unip_acceptor_(new Acceptor(emwp)) {
    seqno_ = 0;
    unip_acceptor_->set_listen_addr(listen_ip_)
        .set_accept_readable_callback([this](
            shared_ptr<TCPConnection>&& shared_p_tc) {
            auto seqno_tmp = seqno_++;
            tcpcon_map_[seqno_tmp] = shared_p_tc;
            if (seqno_cb_ == nullptr) {
                // may happen
            } else {
                seqno_cb_(seqno_tmp);
            }
            tcpcon_map_[seqno_tmp]->set_seqno_of_server(seqno_tmp);
            if (after_connected_ == nullptr) {
                // may happen
            } else {
                after_connected_(*tcpcon_map_[seqno_tmp]);
            }
            tcpcon_map_[seqno_tmp]
                ->set_normal_readable_callback([this, seqno_tmp](
                    TCPConnection& this_con) {
                    if (msg_responser_cb_ != nullptr && msg_cb_ == nullptr) {
                        auto check = this_con.try_to_read();
                        if (check >= 1) {
                            auto& rbuffer = this_con.get_rb_ref();
                            auto& wbuffer = this_con.get_wb_ref();
                            msg_responser_cb_(seqno_tmp, rbuffer, wbuffer,
                                              this_con.get_bigfilesendcb());
                            auto check = this_con.try_to_write();
                            LOG_INFO("server respond %d bytes", check);
                        } else {
                            NOTDONE();  // peer shutdown?
                        }
                    } else if (msg_cb_ != nullptr &&
                               msg_responser_cb_ == nullptr) {
                        msg_cb_(seqno_tmp);
                        auto check = this_con.try_to_write();
                        LOG_INFO("server respond %d bytes", check);
                    } else {
                        ABORT(
                            "did you remember to set callback, or duplicate or "
                            "conflict?");
                    }
                })
                .set_local_close_callback([this](TCPConnection& this_con) {
                    LOG_DEBUG("local close ");
                    remove_tcpcon_by_seqno(this_con.get_seqno());
                })
                .set_peer_close_callback([this](TCPConnection& this_con) {
                    remove_tcpcon_by_seqno(this_con.get_seqno());
                    LOG_DEBUG("peer down");
                })
                .epoll_and_conmunicate();
        })
        .epoll_and_accept();
}

TCPServer::~TCPServer() { LOG_INFO("TCPServer destruct"); }

void TCPServer::remove_tcpcon_by_seqno(uint32_t seqno) {
    auto found = tcpcon_map_.find(seqno);
    if (TCPSTATE::Gooddead == get<1>(*found)->get_state()) {
        tcpcon_map_.erase(found);
    } else {
        NOTDONE();
    }
}

TCPServer& TCPServer::set_tcpcon_after_connected_callback(TCPCallBack&& cb) {
    after_connected_ = std::move(cb);
    return *this;
}

TCPServer& TCPServer::set_accept_get_tcpcon_seqno_callback(
    GetSeqnoCallBack&& cb) {
    seqno_cb_ = std::move(cb);
    return *this;
}

TCPServer& TCPServer::set_msg_responser_callback(MsgResponserCallBack&& cb) {
    msg_responser_cb_ = std::move(cb);
    return *this;
}

TCPServer& TCPServer::set_msg_callback(MsgCallBack&& cb) {
    msg_cb_ = std::move(cb);
    return *this;
}

shared_ptr<TCPConnection>& TCPServer::get_shared_tcpcon_ref_by_seqno(
    uint32_t seqno) {
    auto found = tcpcon_map_.find(seqno);
    if (found != tcpcon_map_.end()) {
        return get<1>(*found);
    } else {
        ABORT(
            "try to get a tcpcon from Invalid seqno, this tcpcon might be "
            "already removed");
    }
}

TCPSTATE TCPServer::get_state() { NOTDONE(); }

TCPClient::TCPClient(EventManagerWrapper* emwp, Ipv4Addr connect_ip,
                     Ipv4Addr local_ip)
    : emp_(emwp->get_pimpl()),
      local_ip_(local_ip),
      connect_ip_(connect_ip),
      unip_connector_(new Connector(emwp)) {
    unip_connector_->set_local_addr(local_ip_)
        .set_connect_to_addr(connect_ip_)
        .set_connect_callback([this](shared_ptr<TCPConnection>&& shared_p_tc) {
            if (tcpcon_ != nullptr) {
                NOTDONE();
            }
            tcpcon_ = shared_p_tc;
            auto seqno_tmp = generate_seqno_of_this_con();
            if (seqno_cb_ == nullptr) {
                // may happen
            } else {
                seqno_cb_(seqno_tmp);
            }
            if (after_connected_ == nullptr) {
                // may happen
            } else {
                after_connected_(*tcpcon_);
            }
            tcpcon_
                    ->set_normal_readable_callback([this, seqno_tmp](
                            TCPConnection& this_con) {
                        if (msg_responser_cb_ != nullptr && msg_cb_ == nullptr) {
                            auto check = this_con.try_to_read();
                            if (check >= 1) {
                                auto& rbuffer = this_con.get_rb_ref();
                                auto& wbuffer = this_con.get_wb_ref();
                                msg_responser_cb_(seqno_tmp, rbuffer, wbuffer,
                                                  this_con.get_bigfilesendcb());
                                // FIXME the best way is every times you read, change it to epollout and in epollout cb, write it.
                                this_con.try_to_write();
                            } else {
                                NOTDONE();
                            }
                        } else if (msg_cb_ != nullptr &&
                                   msg_responser_cb_ == nullptr) {
                            msg_cb_(seqno_tmp);
                        } else {
                            ABORT(
                                    "did you remember to set callback, or duplicate or "
                                            "conflict?");
                        }
                    })
                    .set_peer_close_callback(
                            [](TCPConnection& this_con) { LOG_DEBUG("peer down"); })
                    .set_local_close_callback(
                            [this](TCPConnection& this_con) {
                                LOG_INFO("client local close cb");
                                // TODO do check work before reset it and call the destruct function.
                                tcpcon_.reset();
                            }
                    )
                    .epoll_and_conmunicate();
        })
        .epoll_and_connect();
}

TCPClient::~TCPClient() { LOG_INFO("TCPClient destruct"); }

TCPClient& TCPClient::set_get_tcpcon_seqno_callback(GetSeqnoCallBack&& cb) {
    seqno_cb_ = std::move(cb);
    return *this;
}

TCPClient& TCPClient::set_tcpcon_after_connected_callback(TCPCallBack&& cb) {
    after_connected_ = std::move(cb);
    return *this;
}

TCPClient& TCPClient::set_msg_responser_callback(MsgResponserCallBack&& cb) {
    msg_responser_cb_ = std::move(cb);
    return *this;
}

TCPClient& TCPClient::set_msg_callback(MsgCallBack&& cb) {
    msg_cb_ = std::move(cb);
    return *this;
}

shared_ptr<TCPConnection>& TCPClient::get_shared_tcpcon_ref() {
    return tcpcon_;
}

TCPSTATE TCPClient::get_state() { NOTDONE(); }

uint32_t TCPClient::generate_seqno_of_this_con() {
    // TODO
    return 11111;
}
} /* my_http  */
