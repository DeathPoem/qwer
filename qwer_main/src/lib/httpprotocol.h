#ifndef HTTPPROTOCOL_H
#define HTTPPROTOCOL_H value

#include "tcpserver.h"
#include "nettools.h"

namespace my_http {

// user should not use this class
class HttpMsg {
public:
    HttpMsg();
    virtual ~HttpMsg();

    virtual size_t to_decode(Buffer& buf) = 0;
    virtual size_t to_encode(Buffer& buf) = 0;

protected:
    enum class HttpMethod { Invalid, Get };
    enum class HttpVersion { Invalid, Http1_0, Http1_1 };
    enum class HttpStatusCodes { Invalid, c_200, c_404 };
    HttpMethod str2Method(string s);
    string Method2str(HttpMethod hm);
    HttpVersion str2Version(string s);
    string Version2str(HttpVersion hv);
    HttpStatusCodes str2Status(string s);
    string Status2str(HttpStatusCodes hs);
    const string CRLF_ = "\r\n";
    const string HTML_NEWLINE_ = string(u8"\u8629");
    size_t try_decode_of(Buffer& buffer, std::function<void(string)> const & func, map<int, pair<string, string>> & headers_lines_ref, HttpVersion& version_ref, string & body_ref);
};

class HttpRequest : public HttpMsg {
public:
    HttpRequest();
    virtual ~HttpRequest();
    void get_test_default_one();
    // GET /hello.html HTTP/1.1
    // User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
    // Host: www.tutorialspoint.com
    // Accept-Language: en-us
    // Accept-Encoding: gzip, deflate
    // Connection: Keep-Alive
    virtual size_t to_decode(Buffer& buffer) override;
    virtual size_t to_encode(Buffer& buffer) override;
    void swap(HttpRequest&& other);
    string get_method_str();

    string uri_;
    HttpVersion version_ = HttpVersion::Invalid;
    HttpMethod method_;
    map<int, pair<string, string>> headers_lines_;  // headers can't be empty
    string body_;
};

class HttpResponse : public HttpMsg {
public:
    HttpResponse();
    virtual ~HttpResponse();
    void get_test_default_one();
    // HTTP/1.1 200 OK
    // Date: Mon, 27 Jul 2009 12:28:53 GMT
    // Server: Apache/2.2.14 (Win32)
    // Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
    // Content-Length: 88
    // Content-Type: text/html
    // Connection: Closed
    //<html>
    //<body>
    //<h1>Hello, World!</h1>
    //</body>
    //</html>

    virtual size_t to_decode(Buffer& buffer) override;
    virtual size_t to_encode(Buffer& buffer) override;
    void swap(HttpResponse&& other);

    HttpVersion version_ = HttpVersion::Invalid;
    HttpStatusCodes statuscode_;
    map<int, pair<string, string>> headers_lines_;  // headers can't be empty
    string body_;
};

namespace detail {
template <typename ContainerType>
int splite_by_delimiter(ContainerType const& origin,
                        std::vector<ContainerType>& to_fill_vec,
                        ContainerType const& delimiter) {
    // do nothing
    throw std::runtime_error("this template only support string");
}

using std::string;
template <>
inline int splite_by_delimiter(string const& origin, std::vector<string>& to_fill_vec,
                        string const& delimiter) {
    assert(!delimiter.empty());
    std::vector<decltype(origin.begin())> founds;
    for (auto it = origin.begin(); it != origin.end(); it++) {
        if (*it == delimiter[0]) {
            founds.push_back(it);
        }
    }
    auto pos1 = origin.begin();
    auto pos2 = pos1;
    for (auto f : founds) {
        pos2 = f;
        to_fill_vec.push_back(
            origin.substr(pos1 - origin.begin(), pos2 - pos1));
        pos1 = f;
    }
    pos2 = origin.end();
    to_fill_vec.push_back(origin.substr(pos1 - origin.begin(), pos2 - pos1));
    return founds.size();
}
} /* detail  */

class HttpServer : public noncopyable {
public:
    HttpServer (EventManagerWrapper* emwp, Ipv4Addr listen_ip);
    virtual ~HttpServer ();
    HttpServer& set_action_of(string m_str, string uri, TCPCallBack&& cb);
    HttpServer& set_httpresponse(HttpResponse&& http_response_r_ref);
    void to_encode(Buffer& wb);

private:
    map<pair<string, string>, TCPCallBack> map_;
    HttpResponse http_response_;
    HttpRequest http_request_;
    TCPServer tcp_server_;
};
using X_RF = std::function<void(HttpResponse const &)>;
    using X_SF = std::function<void(string const &)>;
class HttpClient : public noncopyable {
public:
    HttpClient (EventManagerWrapper* emwp, Ipv4Addr local_ip, Ipv4Addr peer_ip);
    virtual ~HttpClient ();
    HttpClient& set_httprequest(HttpRequest&& http_request_r_ref);
    HttpClient& set_httpresponse_msg_collector(X_SF&& cb);   // this is for short TCP link
    HttpClient& set_httpresponse_reflector(X_RF&& hre_ref);

private:
    X_SF msg_collector_;
    X_RF reflector_;
    HttpResponse http_response_;
    HttpRequest http_request_;
    TCPClient tcp_client_;
};
} /* my_http */

#endif /* ifndef HTTPPROTOCOL_H */
