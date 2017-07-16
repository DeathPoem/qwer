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
#include "msg_codec.h"
#include "nettools.h"

namespace my_http {

namespace detail {
int create_socketfd();
//TODO
// tcp keep alive and tcp nodelay
} /* detail */

class TCPConnection;

enum class TCPSTATE {
    Invalid = 0,
    Listening = 1,
    Afterhandshake = 2,  // handshake is happed when listen() a socket, accept()
                         // just return you a new socket
    Tryconnect = 3,
    Gooddead = 4,
    Peerclosed = 5,
    Localclosed = 6,
    Failed = 7,
    Newborn = 8,
    Connected = 9
};

std::ostream& operator<<(std::ostream& os, TCPSTATE state);

using BigFileSendCallBack = std::function<void(string)>;    // this would invoke sendfile() systemcall for large file like pdf to reduce memory copy and effective performance, string would be used to get the full path of file.
using MsgResponserCallBack = std::function<void(uint32_t, Buffer&, Buffer&, BigFileSendCallBack&&)>;    //!< you should read content of rb and write your response into wb, then consume it. using seqno to get reference of this_con.
using MsgCallBack = std::function<void(uint32_t)>;  //!< see MsgResponserCallBack
using SizeCallBack = std::function<size_t()>;
using GetSeqnoCallBack = std::function<void(uint32_t)>;
using MoveTCPConnectionCallBack =
    std::function<void(shared_ptr<TCPConnection>&&)>;
using TCPCallBack = std::function<void(TCPConnection&)>;

class Acceptor {
public:
    Acceptor(EventManagerWrapper* emwp);  // today, I learned that it's a
                                          // constructor dependency injection
    virtual ~Acceptor();
    Acceptor& set_listen_addr(Ipv4Addr addr);
    Acceptor& set_accept_readable_callback(MoveTCPConnectionCallBack&& cb);
    virtual void epoll_and_accept(time_ms_t after = 0);
    TCPSTATE get_state();

protected:
    virtual void listen_it_and_accept();
    virtual void handle_epoll_readable();   //!< an dirty hack to implement MultiAcceptor
    TCPSTATE tcpstate_;
    EventManager* const
        emp_;  // a tcp Acceptor won't live longer than EventManager
    MoveTCPConnectionCallBack movecb_;
    Ipv4Addr listened_ip_;
    unique_ptr<Channel> listened_socket_;
    size_t accepted_count_ = 0;
};

class Connector {
public:
    Connector(EventManagerWrapper* emwp);
    virtual ~Connector();
    Connector& set_local_addr(Ipv4Addr addr);
    Connector& set_connect_to_addr(Ipv4Addr addr);
    Connector& set_connect_stretegy(int flag = 0);
    Connector& set_connect_callback(MoveTCPConnectionCallBack&& cb);
    void epoll_and_connect(time_ms_t after = 0);
    TCPSTATE get_state();

private:
    void connect_it();
    void try_connect_once();    // would connect and epoll wait epollout when EALREADY
    void handle_epoll_connected();
    void handle_epoll_in_good();
    void handle_epoll_in_bad();
    void good_connect_tail_work();
    void close();
    int connect_stretegy_flag_ = 0;
    size_t retry_count_ = 0;
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
    TCPConnection& set_peer_close_callback(TCPCallBack&& cb);       // this cb must not lead to destructor
    TCPConnection& set_local_close_callback(TCPCallBack&& cb);     // this cb can lead to destructor
    TCPConnection& set_idle_callback();
    TCPConnection& set_seqno_of_server(uint32_t seqno);
    void epoll_and_conmunicate();
    // nonblocking read and write
    size_t try_to_write(size_t len) throw(std::runtime_error);
    size_t to_write();
    size_t try_to_read(size_t len);
    size_t to_read();
    void write_by_string(string str);
    string read_by_string();
    Buffer& get_rb_ref();
    Buffer& get_wb_ref();
    Ipv4Addr get_peer();
    uint32_t get_seqno();
    BigFileSendCallBack get_bigfilesendcb();
    void local_close();
    void peer_close();
    TCPSTATE get_state();

private:
    EventManager* const emp_;
    TCPCallBack nread_cb_;
    TCPCallBack nwrite_cb_;
    TCPCallBack peer_close_cb_;
    TCPCallBack local_close_cb_;
    TCPSTATE tcpstate_;
    uint32_t seqno_;
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
    TCPServer(EventManagerWrapper* emwp, Ipv4Addr listen_ip, uint32_t maxtcpcon = 800);
    virtual ~TCPServer();
    TCPServer& set_tcpcon_after_connected_callback(TCPCallBack&& cb); // this interface is not essential
    // using this API, you should store seqno or get_shared_tcpcon_ref_by_seqno and get peer name and register_cb_for_con_of_seqno in msg_responser_.
    TCPServer& set_accept_get_tcpcon_seqno_callback(GetSeqnoCallBack&& cb); // this interface is not essential
    // provide a interface to let outside code to respond, one of following should be used
    TCPServer& set_msg_responser_callback(MsgResponserCallBack&& cb);
    TCPServer& set_tcp_callback(TCPCallBack && cb);
    shared_ptr<TCPConnection>& get_shared_tcpcon_ref_by_seqno(uint32_t seqno);
    TCPSTATE get_state();
    void remove_tcpcon_by_seqno(uint32_t);
private:
    TCPCallBack after_connected_;
    GetSeqnoCallBack seqno_cb_;
    MsgResponserCallBack msg_responser_cb_;
    TCPCallBack tcp_cb_;
    const int maxtcpcon_;
    EventManager* const emp_;
    const Ipv4Addr listen_ip_;
    uint32_t seqno_;
    unique_ptr<Acceptor> unip_acceptor_;
    map<uint32_t, shared_ptr<TCPConnection>> tcpcon_map_;
};

class TCPClient : private noncopyable {
public:
    TCPClient(EventManagerWrapper* emwp, Ipv4Addr connect_ip, Ipv4Addr local_ip);
    virtual ~TCPClient();
    TCPClient& set_tcpcon_after_connected_callback(TCPCallBack&& cb);
    TCPClient& set_get_tcpcon_seqno_callback(GetSeqnoCallBack&& cb);
    TCPClient& set_msg_responser_callback(MsgResponserCallBack&& cb);
    TCPClient& set_tcp_callback(TCPCallBack && cb);
    shared_ptr<TCPConnection>& get_shared_tcpcon_ref();
    TCPSTATE get_state();
private:
    uint32_t generate_seqno_of_this_con();
    TCPCallBack after_connected_;
    GetSeqnoCallBack seqno_cb_;
    MsgResponserCallBack msg_responser_cb_;
    TCPCallBack  tcp_cb_;
    Ipv4Addr connect_ip_;
    Ipv4Addr local_ip_;
    EventManager* const emp_;
    unique_ptr<Connector> unip_connector_;
    shared_ptr<TCPConnection> tcpcon_;
};

} /* my_http */

#endif /* ifndef TCPSERVER_H */
