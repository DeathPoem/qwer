#include "nettools.h"
#include <cstring>
#include "facility.h"

namespace my_http {
Ipv4Addr::Ipv4Addr() {}

Ipv4Addr::Ipv4Addr(const struct sockaddr_in& addr_arg) : addr_(addr_arg) {}

bool Ipv4Addr::operator==(const Ipv4Addr& rhs) const {
    auto rhs_addr_in = rhs.get_socketaddr_in();
    return addr_.sin_addr.s_addr == rhs_addr_in.sin_addr.s_addr &&
           addr_.sin_port == rhs_addr_in.sin_port;
}

int Ipv4Addr::hostname_2_ip(char* hostname, char* ip) {
    struct hostent* he;
    struct in_addr** addr_list;
    if ((he = gethostbyname(hostname)) == NULL) {
        // get the host info
        int iii = 1;
        LOG_ERROR("gethostbyname,nomeaning =%d", iii);
        return 1;
    }
    addr_list = (struct in_addr**)he->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        // Return the first one;
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }
    return 1;
}

string Ipv4Addr::host2ip_str(const string& host) {
    char ipcstr[100], hostnamecstr[100];
    strcpy(hostnamecstr, host.c_str());
    if (Ipv4Addr::hostname_2_ip(hostnamecstr, ipcstr) == 0) {
        return string(ipcstr);
    } else {
        return "";
    }
}

struct sockaddr_in Ipv4Addr::get_socketaddr_in() const {
    return addr_;
}

sockaddr* Ipv4Addr::get_p_socketaddr() { return (sockaddr*)(&addr_); }

int Ipv4Addr::ip_bind_socketfd(int fd) {
    int ret = ::bind(fd, (struct sockaddr*)&addr_, sizeof(addr_));
    if (ret < 0) {
        SLOG_DEBUG("errorno=" << std::strerror(errno));
        throw std::system_error(errno, std::system_category());
    }
    return ret;
}

Ipv4Addr::Ipv4Addr(string ipdotdecimal, int port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    string ip_str = ipdotdecimal;
    // string ip_str = Ipv4Addr::host2ip_str(host);
    if (ipdotdecimal == "255.255.255.255") {
        addr_.sin_addr.s_addr = INADDR_ANY;
    } else if (!ip_str.empty()) {
        addr_.sin_addr.s_addr = inet_addr(ip_str.c_str());
    } else {
        LOG_ERROR("not able to resolve hostname 2 ip");
    }
}

string Ipv4Addr::get_ip_str() { return string(inet_ntoa(addr_.sin_addr)); }

Buffer::Buffer()
    : buffer_(vector<char>(initial_size_, 'a')),
      begin_(0),
      end_(0),
      capacity_(buffer_.size()) {}

Buffer::Buffer(const Buffer& rhs) {
    buffer_ = rhs.buffer_;
    begin_ = rhs.begin_;
    end_ = rhs.end_;
    capacity_ = rhs.capacity_;
}

Buffer::~Buffer() {}

void Buffer::swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    begin_ = rhs.begin_;
    end_ = rhs.end_;
    capacity_ = rhs.capacity_;
}

Buffer& Buffer::operator=(const Buffer& rhs) {
    if (this == &rhs) {
        return *this;
    }
    buffer_ = rhs.buffer_;
    begin_ = rhs.begin_;
    end_ = rhs.end_;
    capacity_ = rhs.capacity_;
    return *this;
}

size_t Buffer::size() const { return buffer_.size(); }

bool Buffer::invariant_check() throw(std::runtime_error) {
    capacity_ = buffer_.size();
    bool checked = begin_ <= end_ && end_ <= capacity_;
    if (checked && do_you_remember_consume_or_do_not_consume_) {
        return true;
    } else {
        throw std::runtime_error("invariant of Buffer is broken");
    }
}

size_t Buffer::get_readable_bytes() const { return end_ - begin_; }

size_t Buffer::get_writable_bytes() const { return capacity_ - end_; }

size_t Buffer::write_to_buffer(char* writefrom, size_t len) {
    invariant_check_wrap();
    char* writeto = get_end();
    if (get_writable_bytes() < len) {
        buffer_.resize(buffer_.size() + len);
    }
    std::memcpy(writeto, writefrom, len);
    end_ += len;
    return len;
}

size_t Buffer::write_to_buffer(int fd, size_t len) {
    invariant_check_wrap();
    char* writeto = get_end();
    if (get_writable_bytes() < len) {
        buffer_.resize(buffer_.size() + len);
    }
    int left = len;
    while (left > 0) {
        auto n = ::read(fd, writeto, len);  // it's also able to use readn() 
        if (n < 0) {
            if (errno == EINTR) {
                n = 0;
            } else {
                NOTDONE();
            }
        } else if (n == 0) {
            // EOF
            break;
        }
        left -= n;
        writeto += n;
    }
    end_ += len;
    return len - left;
}

void Buffer::invariant_check_wrap() throw(std::runtime_error) {
        invariant_check();
}

void Buffer::consume(size_t len) {
    begin_ += len;
    if (!do_you_remember_consume_or_do_not_consume_) {
        do_you_remember_consume_or_do_not_consume_ = true;
    } else {
        throw std::runtime_error("duplicate enter consume");
    }
}

void Buffer::do_not_consume(size_t nomeaning) {
    if (!do_you_remember_consume_or_do_not_consume_) {
        do_you_remember_consume_or_do_not_consume_ = true;
    } else {
        throw std::runtime_error("duplicate enter consume");
    }
}

size_t Buffer::read_from_buffer(int fd, size_t len) throw(std::runtime_error) {
    invariant_check_wrap();
    if (do_you_remember_consume_or_do_not_consume_) {
        do_you_remember_consume_or_do_not_consume_ = false;
    } else {
        throw std::runtime_error("not consume and read rightly");
    }
    char* readfrom = get_begin();
    int left = len;
    if (get_readable_bytes() >= len) {
        while (left > 0) {
            auto n = ::write(fd, readfrom, len);
            if (n < 0) {
                if (errno == EINTR) {
                    n = 0;
                } else {
                    NOTDONE();
                }
            }
            left -= n;
            readfrom += n;
        }
    } else {
        throw std::runtime_error("buffer read to much");
    }
    return len - left;
}

size_t Buffer::read_from_buffer(char* readto,
                                 size_t len) throw(std::runtime_error) {
    invariant_check_wrap();
    if (do_you_remember_consume_or_do_not_consume_) {
        do_you_remember_consume_or_do_not_consume_ = false;
    } else {
        throw std::runtime_error("not consume and read rightly");
    }
    char* readfrom = get_begin();
    if (get_readable_bytes() >= len) {
        std::memcpy(readto, readfrom, len);
    } else {
        throw std::runtime_error("buffer read to much");
    }
    return len;
}

char* Buffer::get_begin() { return &(*buffer_.begin()) + begin_; }

char* Buffer::get_end() { return &(*buffer_.begin()) + end_; }
} /* my_http */
