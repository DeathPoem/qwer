#include "tcpserver.h"
namespace my_http {

Acceptor::Acceptor(EventManagerWrapper* emwp)
    : emp_(emwp->get_pimpl()),
      listened_socket_(new Channel(detail::create_socketfd())) {
    int listen_fd = listened_socket_->get_fd();
    if (listen_fd < 0) {
        NOTDONE();
    }
    int flags = fcntl(listen_fd, F_GETFL, 0);
    if (flags < 0) {
        NOTDONE();
    }
    auto check = fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);
    if (check < 0) {
        NOTDONE();
    }
    tcpstate_ = TCPSTATE::Newborn;
}

Acceptor::~Acceptor() {}

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

void Acceptor::listen_it() {
    listened_ip_.ip_bind_socketfd(listened_socket_->get_fd());
    auto check = ::listen(listened_socket_->get_fd(), SOMAXCONN);
    tcpstate_ = TCPSTATE::listening;
    listened_socket_->set_events(Channel::get_readonly_event_flag());
    if (check != 0) {
        NOTDONE();
    } else {
    }
}

void Acceptor::handle_epoll_readable() {
    // when socketfd is readable, you can accept new fd
    int new_fd;
    Ipv4Addr peer;
    unsigned int len = sizeof(peer.get_p_socketaddr());
    while (listened_socket_->get_fd() &&
           (new_fd = ::accept(listened_socket_->get_fd(),
                              peer.get_p_socketaddr(), &len)) >= 0) {
        // accept all socket, then put them to others, by cb_
        if (new_fd < 0) {
            // no more
            break;
        }
        auto check = ::getpeername(listened_socket_->get_fd(),
                                   peer.get_p_socketaddr(), &len);
        if (check != 0) {
            NOTDONE();
        }
        shared_ptr<TCPConnection> shared_p_tc(
            new TCPConnection(emp_, new_fd, listened_ip_, peer));
        movecb_(shared_p_tc);
    }
}

void Acceptor::epoll_and_accept(time_ms_t after) {
    emp_->run_after(after, std::bind(&Acceptor::listen_it, this));
    if (movecb_ != nullptr && (listened_socket_->get_events() & EPOLLIN)) {
        emp_->register_event(listened_socket_.get(), [this]() {
            SLOG_INFO("in Acceptor::accept callback, accepted_count_ ="
                      << accepted_count_);
            handle_epoll_readable();
        });
    } else {
        NOTDONE();
    }
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
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        NOTDONE();
    }
    auto check = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (check < 0) {
        NOTDONE();
    }
    tcpstate_ = TCPSTATE::Newborn;
}

Connector::~Connector() {}

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
    local_ip_.ip_bind_socketfd(to_connect_socket_->get_fd());
    if (connect_stretegy_flag_ == 0) {
        auto check = try_connect_once();
        if (check) {
            shared_ptr<TCPConnection> shared_p_tc(
                new TCPConnection(emp_, std::move(to_connect_socket_),
                                  local_ip_, to_connect_ip_));
            movecb_(shared_p_tc);
        }
    } else {
        // may try to reconnect
        NOTDONE();
    }
}

void Connector::close() {
    if (to_connect_socket_ == nullptr) {
        tcpstate_ = TCPSTATE::Connected;
    } else {
        tcpstate_ = TCPSTATE::Failed;
    }
}

void Connector::handle_epoll_connected() {
    connect_it();
    close();
}

Connector& Connector::set_connect_stretegy(int flag) {
    connect_stretegy_flag_ = flag;
    return *this;
}

bool Connector::try_connect_once() {
    unsigned int len = sizeof(to_connect_ip_.get_p_socketaddr());
    int check = ::connect(to_connect_socket_->get_fd(),
                          to_connect_ip_.get_p_socketaddr(), len);
    if (check != 0) {
        NOTDONE();
    } else {
        return true;
    }
}

void Connector::epoll_and_connect(time_ms_t after) {
    emp_->run_after(after, std::bind(&Connector::handle_epoll_connected, this));
}

TCPConnection::TCPConnection(EventManager* emp, int fd, Ipv4Addr local,
                             Ipv4Addr peer)
    : local_(local), peer_(peer), unique_p_ch_(new Channel(fd)) {
    tcpstate_ = TCPSTATE::Afterhandshake;
}

TCPConnection::TCPConnection(EventManager* emp, unique_ptr<Channel> socket_ch,
                             Ipv4Addr local, Ipv4Addr peer)
    : local_(local), peer_(peer), unique_p_ch_(std::move(socket_ch)) {
    tcpstate_ = TCPSTATE::Afterhandshake;
}

TCPConnection::~TCPConnection() {}

TCPConnection& TCPConnection::set_normal_readable_callback(TCPCallBack&& cb) {
    nread_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_normal_writable_callback(TCPCallBack&& cb) {
    nwrite_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_peer_close_readable_callback(
    TCPCallBack&& cb) {
    close_cb_ = std::move(cb);
    return *this;
}

TCPConnection& TCPConnection::set_idle_callback() { return *this; }

void TCPConnection::epoll_and_conmunicate() {
    if (nread_cb_ != nullptr && (unique_p_ch_->get_events() & EPOLLIN)) {
        emp_->register_event(unique_p_ch_.get(),
                             [this]() { handle_epoll_readable(); });
    }
    if (nwrite_cb_ != nullptr && (unique_p_ch_->get_events() & EPOLLOUT)) {
        emp_->register_event(unique_p_ch_.get(),
                             [this]() { handle_epoll_readable(); });
    }
    if (close_cb_ != nullptr && (unique_p_ch_->get_events() & EPOLLRDHUP)) {
        emp_->register_event(unique_p_ch_.get(),
                             [this]() { handle_epoll_readable(); });
    }
}

void TCPConnection::handle_epoll_readable() { nread_cb_(*this); }

void TCPConnection::handle_epoll_peer_shut_down() {
    tcpstate_ = TCPSTATE::Peerclosed;
    close_cb_(*this);
    close();
}

void TCPConnection::handle_epoll_writable() { nwrite_cb_(*this); }

Buffer& TCPConnection::get_rb_ref() { return read_sock_to_this_; }

Buffer& TCPConnection::get_wb_ref() { return write_sock_from_this_; }

Ipv4Addr TCPConnection::get_peer() {
    return peer_;
}

size_t TCPConnection::try_to_read(size_t len) {
    int nread = read_sock_to_this_.write_to_buffer(unique_p_ch_->get_fd(), len);
    if (nread == len) {
        return nread;
    } else {
        NOTDONE();
    }
}

size_t TCPConnection::try_to_read() {
    size_t len;
    int check = ::ioctl(unique_p_ch_->get_fd(), FIONREAD, &len);
    return try_to_read(len);
}

string TCPConnection::read_by_string() {
    char str_c[1000];
    int nread = try_to_read();
    assert(1000 > nread);
    int check = read_sock_to_this_.read_from_buffer(str_c, nread);
    if (nread == check) {
        return string(str_c);
    } else {
        NOTDONE();
    }
}

void TCPConnection::write_by_string(string str) {
    char* str_c = &str[0];
    int nwrite = write_sock_from_this_.write_to_buffer(str_c, str.size());
    assert(nwrite == str.size());
    auto check = try_to_write();
    if (check == nwrite) {
    } else {
        NOTDONE();
    }
}

size_t TCPConnection::try_to_write() {
    int len = write_sock_from_this_.get_readable_bytes();
    return try_to_write(len);
}

size_t TCPConnection::try_to_write(size_t len) {
    int nwrite =
        write_sock_from_this_.read_from_buffer(unique_p_ch_->get_fd(), len);
    if (nwrite == len) {
        return nwrite;
    } else {
        NOTDONE();
        return -1;
    }
}

void TCPConnection::close() { LOG_INFO("one TCPConnection close"); }

TCPServer::TCPServer(EventManagerWrapper* emwp, MsgResponser* msg_responser,
                     Ipv4Addr listen_ip, uint32_t maxtcpcon, bool period_remove_expired_tcpcon_flag)
    : emp_(emwp->get_pimpl()),
      listen_ip_(listen_ip),
      msg_responser_(msg_responser),
      maxtcpcon_(maxtcpcon),
      unip_acceptor_(new Acceptor(emwp)) {
    seqno_ = 0;
    if (period_remove_expired_tcpcon_flag) {
        period_remove_expired_tcpcon();
    }
    unip_acceptor_->set_listen_addr(listen_ip_)
        .set_accept_readable_callback([this](
            shared_ptr<TCPConnection> shared_p_tc) {
            tcpcon_map_[seqno_] = shared_p_tc;
            auto seqno_tmp = seqno_;
            if (cb_ != nullptr) {
                cb_(seqno_++);
            } else {
                NOTDONE();
            }
            shared_p_tc
                ->set_normal_readable_callback([this, seqno_tmp](TCPConnection& this_con) {
                    auto check = this_con.try_to_read(
                        msg_responser_->get_require_size());
                    if (check >= msg_responser_->get_require_size()) {
                        auto& rbuffer = this_con.get_rb_ref();
                        auto& wbuffer = this_con.get_wb_ref();
                        msg_responser_->do_it_for_con_of_seqno(seqno_tmp, rbuffer, wbuffer);
                        this_con.try_to_write();
                    } else {
                        NOTDONE();
                    }
                })
                .set_peer_close_readable_callback(
                    [](TCPConnection& this_con) { LOG_DEBUG("peer down"); })
                .epoll_and_conmunicate();
        })
        .epoll_and_accept();
}

TCPServer::~TCPServer() {}

TCPServer& TCPServer::set_accept_get_tcpcon_seqno_callback(GetSeqnoCallBack&& cb) {
    cb_ = std::move(cb);
    return *this;
}

shared_ptr<TCPConnection>& TCPServer::get_shared_tcpcon_ref_by_seqno(uint32_t seqno) {
    auto found = tcpcon_map_.find(seqno);
    if (found != tcpcon_map_.end()) {
        return get<1>(*found);
    } else {
        ABORT("try to get a tcpcon from Invalid seqno, this tcpcon might be already removed");
    }
}

void TCPServer::period_remove_expired_tcpcon() {
    emp_->run_after(1000, [this](){
                remove_expired_tcpcon_once();
                period_remove_expired_tcpcon();
            });
}

void TCPServer::remove_expired_tcpcon_once() { }
} /* my_http  */
