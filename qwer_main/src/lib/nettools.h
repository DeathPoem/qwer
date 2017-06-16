#ifndef NETTOOLS
#define NETTOOLS value
#include <arpa/inet.h>
#include "facility.h"

namespace my_http {
struct Ipv4Addr {
    Ipv4Addr();
    Ipv4Addr(string ipdotdecimal, int port);
    Ipv4Addr(const struct sockaddr_in& addr_arg);
    string get_ip_str();
    sockaddr_in get_socketaddr_in() const;
    sockaddr* get_p_socketaddr();
    static string host2ip_str(const string& host);
    int ip_bind_socketfd(int fd);
    bool operator==(const Ipv4Addr& rhs) const;

private:
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
    size_t write_to_buffer(char* writefrom, size_t len);
    size_t read_from_buffer(char* readto,
                             size_t len) throw(std::runtime_error);
    // blocking system call, handle EINTR
    size_t write_to_buffer(int fd, size_t len);
    size_t read_from_buffer(int fd, size_t len) throw(std::runtime_error);
    // app consume, you must call either of this two after read from this buffer, if not, invariant_check would fall next time use it
    void consume(size_t len);
    void do_not_consume(size_t nomeaning);
private:
    bool do_you_remember_consume_or_do_not_consume_ = true;
    size_t size() const;
    char* get_begin();
    char* get_end();
    const size_t initial_size_ = 1024;
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
int read_from_Channel_to_Buffer(const Channel& ch, Buffer& buf);
}
}
#endif /* ifndef NETTOOLS */
