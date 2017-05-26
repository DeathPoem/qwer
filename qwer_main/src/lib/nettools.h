#ifndef NETTOOLS
#define NETTOOLS value
#include "facility.h"
#include <arpa/inet.h>

namespace my_http {
    struct Ipv4Addr {
        //Ipv4Addr(string host, int port);
        Ipv4Addr(string ipdotdecimal, int port);
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
        Buffer& operator=(const Buffer& rhs); 
        ~Buffer();
        void swap(Buffer& rhs);
        size_t get_readable_bytes() const;
        size_t get_writable_bytes() const;
        Buffer& write_to_buffer(char* writefrom, size_t len);
        Buffer& read_from_buffer(char* readto, size_t len) throw(std::runtime_error);

        private :
        size_t size() const;
        char* get_begin();
        char* get_end();
        static const size_t space_append_size_ = 64;
        // TODO add own allocator
        vector<char> buffer_;
        bool invariant_check() throw(std::runtime_error);
        void invariant_check_wrap();
        size_t begin_, end_, capacity_;
    };

    struct Socketfd {

    };

}
#endif /* ifndef NETTOOLS */
