#ifndef TCPSERVER_H
#define TCPSERVER_H value

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "epollwrapper.h"
#include "event.h"
#include "msg_responser.h"
#include "nettools.h"

namespace my_http {

namespace detail {
int create_socketfd();
} /* detail */

class TCPConnection;

enum class TCPSTATE {
    Invalid = 0,
    listening = 7,
    Afterhandshake = 1,  // handshake is happed when listen() a socket, accept()
                         // just return you a new socket
    try_connect = 6,
    Connected = 2,
    Peerclosed = 3,
    Localclosed = 4,
    Failed = 5,
    Newborn = 8
};

using GetSeqnoCallBack = std::function<void(uint32_t)>;
using MoveTCPConnectionCallBack =
    std::function<void(shared_ptr<TCPConnection>)>;
using TCPCallBack = std::function<void(TCPConnection&)>;

class Acceptor {
public:
    Acceptor(EventManagerWrapper* emwp);  // today, I learned that it's a
                                          // constructor dependency injection
    virtual ~Acceptor();
    Acceptor& set_listen_addr(Ipv4Addr addr);
    Acceptor& set_accept_readable_callback(MoveTCPConnectionCallBack&& cb);
    void epoll_and_accept(time_ms_t after = 0);
    TCPSTATE get_state();

private:
    void listen_it();
    void handle_epoll_readable();
    TCPSTATE tcpstate_;
    EventManager* const
        emp_;  // a tcp Acceptor won't live longer than EventManager
    MoveTCPConnectionCallBack movecb_;
    Ipv4Addr listened_ip_;
    unique_ptr<Channel> listened_socket_;
    size_t accepted_count_;
};

class Connector {
public:
    Connector(EventManagerWrapper* emwp);
    virtual ~Connector();
    Connector& set_local_addr(Ipv4Addr addr);
    Connector& set_connect_to_addr(Ipv4Addr addr);
    Connector& set_connect_callback(MoveTCPConnectionCallBack&& cb);
    Connector& set_connect_stretegy(int flag = 0);
    void epoll_and_connect(time_ms_t after = 0);
    TCPSTATE get_state();

private:
    void connect_it();
    bool try_connect_once();
    void handle_epoll_connected();
    void close();
    int connect_stretegy_flag_;
    TCPSTATE tcpstate_;
    EventManager* emp_;
    MoveTCPConnectionCallBack movecb_;
    Ipv4Addr local_ip_;
    Ipv4Addr to_connect_ip_;
    unique_ptr<Channel> to_connect_socket_;
};

class TCPConnection : public std::enable_shared_from_this<TCPConnection>,
                      private noncopyable {
public:
    TCPConnection(EventManager* emp, int fd, Ipv4Addr local, Ipv4Addr peer);
    TCPConnection(EventManager* emp, unique_ptr<Channel> socket_ch,
                  Ipv4Addr local, Ipv4Addr peer);
    virtual ~TCPConnection();
    TCPConnection& set_normal_readable_callback(TCPCallBack&& cb);
    TCPConnection& set_normal_writable_callback(TCPCallBack&& cb);
    TCPConnection& set_peer_close_readable_callback(TCPCallBack&& cb);
    TCPConnection& set_idle_callback();
    void epoll_and_conmunicate();
    // nonblocking read and write
    size_t try_to_write(size_t len);
    size_t try_to_write();
    size_t try_to_read(size_t len);
    size_t try_to_read();
    void write_by_string(string str);
    string read_by_string();
    Buffer& get_rb_ref();
    Buffer& get_wb_ref();
    Ipv4Addr get_peer();
    void close();
    TCPSTATE get_state();

private:
    EventManager* const emp_;
    TCPCallBack nread_cb_;
    TCPCallBack nwrite_cb_;
    TCPCallBack close_cb_;
    TCPSTATE tcpstate_;
    Buffer read_sock_to_this_, write_sock_from_this_;
    void handle_epoll_readable();
    void handle_epoll_writable();
    void handle_epoll_peer_shut_down();
    unique_ptr<Channel> unique_p_ch_;
    Ipv4Addr local_;
    Ipv4Addr peer_;
};

class TCPServer : private noncopyable {
public:
    TCPServer(EventManagerWrapper* emwp, MsgResponser* msg_responser,
              Ipv4Addr listen_ip, uint32_t maxtcpcon = 800, bool period_remove_expired_tcpcon_flag = false);
    virtual ~TCPServer();
    TCPServer& set_accept_get_tcpcon_seqno_callback(GetSeqnoCallBack&& cb); // you should store seqno or get_shared_tcpcon_ref_by_seqno and get peer name and register_cb_for_con_of_seqno in msg_responser_ in this cb.
    // unique_ptr<Acceptor>& get_uni_acceptor_ref();
    shared_ptr<TCPConnection>& get_shared_tcpcon_ref_by_seqno(uint32_t seqno);

private:
    void period_remove_expired_tcpcon();
    void remove_expired_tcpcon_once();
    const int maxtcpcon_;
    EventManager* const emp_;
    MsgResponser* const msg_responser_;
    const Ipv4Addr listen_ip_;
    uint32_t seqno_;
    GetSeqnoCallBack cb_;
    unique_ptr<Acceptor> unip_acceptor_;
    map<uint32_t, shared_ptr<TCPConnection>> tcpcon_map_;
};

class TCPClient : private noncopyable {
public:
    TCPClient();
    virtual ~TCPClient();

private:
    unique_ptr<Connector> unip_connector_;
    shared_ptr<TCPConnection> tcpcon_;
};

} /* my_http */

#endif /* ifndef TCPSERVER_H */
