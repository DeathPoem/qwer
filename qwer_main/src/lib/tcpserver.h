#ifndef TCPSERVER_H
#define TCPSERVER_H value

#include "nettools.h"
#include "epollwrapper.h"

namespace my_http {

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
