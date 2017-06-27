#ifndef NETTOOLS
#define NETTOOLS value
#include <arpa/inet.h>
#include "facility.h"

namespace my_http {
struct Ipv4Addr {
    Ipv4Addr();
    Ipv4Addr(string ipdotdecimal, int port);
    Ipv4Addr(struct sockaddr_in const & addr_arg);
    string get_ip_str();
    sockaddr_in get_socketaddr_in() const;
    sockaddr* get_p_socketaddr();
    static string host2ip_str(string const & host);
    int ip_bind_socketfd(int fd);
    bool operator==(Ipv4Addr const & rhs) const;

private:
    static int hostname_2_ip(char* hostname, char* ip);
    sockaddr_in addr_;
};

struct Buffer {
    Buffer();
    Buffer(Buffer const & rhs);
    Buffer& operator=(Buffer const & rhs);
    ~Buffer();
    void swap(Buffer& rhs);
    size_t get_readable_bytes() const;
    size_t get_writable_bytes() const;
    size_t write_to_buffer(char const * writefrom, size_t len);
    size_t read_from_buffer(char* readto,
                             size_t len) throw(std::runtime_error);
    // blocking system call, handle EINTR
    size_t write_to_buffer(int fd, size_t len);
    size_t read_from_buffer(int fd, size_t len) throw(std::runtime_error);
    // app consume, you must call either of this two after read from this buffer, if not, invariant_check would fall next time use it
    void consume(size_t len);
    void do_not_consume(size_t nomeaning = 0);
private:
    bool do_you_remember_consume_or_do_not_consume_ = true;
    size_t size() const;
    char* get_begin();      // yes, if I expose this method, it would be more effective, due to the less copy we should do. But, keep it private would let code more readable. FIXME
    char* get_end();
    size_t const initial_size_ = 1024;
    // static const size_t space_append_size_ = 64;
    // TODO add own allocator
    vector<char> buffer_;
    bool invariant_check() throw(std::runtime_error);
    void invariant_check_wrap() throw(std::runtime_error);
    size_t begin_, end_, capacity_;
    //                 begin_              end_      capacity_    
    //    used& consumed | unconsumed &  not used |         |
};

namespace detail {
int read_from_Channel_to_Buffer(Channel const & ch, Buffer& buf);
}
}
#endif /* ifndef NETTOOLS */
