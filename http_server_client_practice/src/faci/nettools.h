#ifndef NETTOOLS
#define NETTOOLS value
#include "facility.h"
#include <arpa/inet.h>

namespace my_http {
    struct Ipv4Addr {
        Ipv4Addr(string host, int port);
        Ipv4Addr(const struct sockaddr_in& addr_arg);
        string get_ip_str();
        sockaddr_in* get_socketaddr_in();
        static string host2ip_str(const string& host);
        private :
        static int hostname_2_ip(char* hostname, char* ip);
        sockaddr_in addr_;
    };

    struct Buffer {
        Buffer();
        Buffer(const Buffer& rhs);
        Buffer& operator=(const Buffer& rhs) {
            
        }
        ~Buffer();
        size_t size();
        bool empty();
        char* get_data();
        char* get_begin();
        char* get_end();
        size_t get_readable_bytes();
        size_t get_writable_bytes();
        private :
        char* alloc_space(size_t space_size);
        char* buffer_;
        size_t begin_, end_, capacity_;
    };
}
#endif /* ifndef NETTOOLS */
