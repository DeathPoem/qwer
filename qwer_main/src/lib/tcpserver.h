#ifndef TCPSERVER_H
#define TCPSERVER_H value

#include "nettools.h"
#include "epollwrapper.h"
#include "event.h"

namespace my_http {

    class Acceptor {
        public:
            Acceptor (EventManagerWrapper* emwp);
            virtual ~Acceptor ();
            Acceptor& set_listen_addr();
            Acceptor& set_accept_callback();

        private:
            EventManager* emp_; // a tcp Acceptor won't live longer than EventManager
            CallBack cb_;
            unique_ptr<Channel> listened_socket_;
            unique_ptr<Channel> new_socket_;
    };

    class TCPSession : public std::enable_shared_from_this<TCPSession>, private noncopyable {
        public:
            TCPSession ();
            virtual ~TCPSession ();

        private:
            unique_ptr<Channel> uni_ch_;
    };

    class TCPServer : private noncopyable {
        public:
            TCPServer ();
            virtual ~TCPServer ();

        private:
            /* data */
    };

} /* my_http */ 

#endif /* ifndef TCPSERVER_H */
