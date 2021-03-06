#ifndef HTTPPROTOCOL_H
#define HTTPPROTOCOL_H value

#include "tcpserver.h"
#include "nettools.h"
#include "multithread_version.h"
//#include "../third_party/http-parser/http_parser.h"
#include "http_parser.h"

namespace my_http {

class HttpMsg {
public:
    HttpMsg();
    virtual ~HttpMsg();

    virtual size_t to_decode(Buffer& buf) = 0;
    virtual size_t to_encode(Buffer& buf) = 0;
    enum class HttpMethod { Invalid, Get };
    enum class HttpVersion { Invalid, Http1_0, Http1_1 };
    enum class HttpStatusCodes { Invalid, c_200, c_404 };

    static const string CRLF_;
    static const string HTML_NEWLINE_;
protected:
    HttpMethod str2Method(string s);
    string Method2str(HttpMethod hm);
    HttpVersion str2Version(string s);
    string Version2str(HttpVersion hv);
    HttpStatusCodes str2Status(string s);
    string Status2str(HttpStatusCodes hs);
    size_t try_decode_of(Buffer& buffer, std::function<void(string)> const & func, vector<pair<string, string>> & headers_lines_ref, HttpVersion& version_ref, string & body_ref);
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
    vector<pair<string, string>> headers_lines_;  // headers can't be empty
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
    vector<pair<string, string>> headers_lines_;  // headers can't be empty
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


template <typename TCPServerClass>
class HttpServer : public noncopyable {
public:
    HttpServer (EventManagerWrapper* emwp, Ipv4Addr listen_ip);
    HttpServer (size_t idle, Ipv4Addr listen_ip);
    virtual ~HttpServer ();
    HttpServer& set_action_of(string m_str, string uri, TCPCallBack&& cb);
    HttpServer& set_httpresponse(HttpResponse&& http_response_r_ref);
    //void to_encode(Buffer& wb);

private:
    map<pair<string, string>, TCPCallBack> map_;
    HttpResponse http_response_;
    HttpRequest http_request_;
    unique_ptr<TCPServerClass> tcp_server_p_;
};

using X_RF = std::function<void(HttpResponse const &)>;
    using X_SF = std::function<void(string const &)>;

template <typename TCPClientClass>
class HttpClient : public noncopyable {
public:
    HttpClient (EventManagerWrapper* emwp, Ipv4Addr local_ip, Ipv4Addr peer_ip);
    virtual ~HttpClient ();
    HttpClient& set_httprequest(HttpRequest&& http_request_r_ref);
    HttpClient& set_httpresponse_msg_collector(X_SF&& cb);
    HttpClient& set_httpresponse_reflector(X_RF&& hre_ref);

private:
    X_SF msg_collector_;
    X_RF reflector_;
    HttpResponse http_response_;
    HttpRequest http_request_;
    unique_ptr<TCPClientClass> tcp_client_p_;
};

    //! read httprequest and write your httpresponse, then call to_lazy_close() if you want a non-persistent connection( close socket after response)
    //! this callback should be reentrancy one
    using HttpCallBack = function<void(TCPConnection&, shared_ptr<HttpRequest>, shared_ptr<HttpResponse>)>;  //!< each request is created by each parse, but response can be a cashed object.
    using NextCallBack = function<void()>;
    using MiddleWareCb = function<void(TCPConnection&, shared_ptr<HttpRequest>, shared_ptr<HttpResponse>, NextCallBack)>;

    //! thread safe http api holder and http request handler, thread safe for read operation
    class HttpSetting {
    public:
        HttpSetting();
        virtual ~HttpSetting();
        //! this action should not block
        HttpSetting& set_action_of(string method, string uri, HttpCallBack&& cb);   //!< lock free
        HttpSetting& cash_response_of(string method, string uri, shared_ptr<HttpResponse> res_p);
        void routeit(TCPConnection& this_con, shared_ptr<HttpRequest> http_request, shared_ptr<HttpResponse> http_response);   //!< lock free until invoke HttpCallBack if HttpCallBack is not thread safe
        void middle_seq_of(shared_ptr<HttpRequest> http_request, TCPConnection& this_con);   
        HttpSetting& use(MiddleWareCb&& cb);
    private:
        mutex mutex_;
        vector<MiddleWareCb> middle_seq_;
        map<pair<string, string>, HttpCallBack> map_;
        map<pair<string, string>, shared_ptr<HttpResponse>> map_res_;
    };

template <>
class HttpServer<MultiServer> : public noncopyable {
public:
    HttpServer (size_t idle, Ipv4Addr listen_ip, shared_ptr<HttpSetting> httpsetting);
    virtual ~HttpServer ();
    void Start(function<bool()>&& checker = nullptr); //!< wait for checher return true
    void Exit();
private:
    shared_ptr<HttpSetting> p_httpsetting_;
    unique_ptr<MultiServer> tcp_server_p_;
};



template <typename TCPClientClass>
HttpClient<TCPClientClass>::HttpClient(EventManagerWrapper* emwp, Ipv4Addr local_ip,
                       Ipv4Addr peer_ip)
    : tcp_client_p_(new TCPClientClass(emwp, peer_ip, local_ip)) {
    tcp_client_p_
        ->set_tcpcon_after_connected_callback([this](TCPConnection& this_con) {
            LOG_DEBUG("httpclient connect callback");
            auto& wb = this_con.get_wb_ref();
            auto check = http_request_.to_encode(wb);
            auto check1 = this_con.to_write();
            if (check != check1 || check == 0) { NOTDONE(); }
        })
        .set_tcp_callback([this](TCPConnection& this_con) {
            if (msg_collector_ && !reflector_) {
                LOG_DEBUG("httpclient tcp callback");
                this_con.get_rb_ref().consume(
                    http_response_.to_decode(this_con.get_rb_ref()));
                msg_collector_(http_response_.body_);
            } else if (reflector_ && !msg_collector_) {
                NOTDONE();
            } else {
                NOTDONE();
            }
        });
}

template <typename TCPServerClass>
HttpServer<TCPServerClass>::HttpServer(EventManagerWrapper* emwp, Ipv4Addr listen_ip)
    : tcp_server_p_(new TCPServerClass(emwp, listen_ip)) {
        tcp_server_p_->set_tcp_callback([this](TCPConnection& this_con) {
                    LOG_DEBUG("httpserver tcp callback");
                    this_con.get_rb_ref().consume(
                            http_request_.to_decode(this_con.get_rb_ref()));
                    pair<string, string> key = make_pair(
                            http_request_.get_method_str(),
                            http_request_.uri_);
                    assert(!map_.empty());
                    auto found = map_.find(key);
                    if (found != map_.end()) {
                        auto& func_ref = map_[key];
                        func_ref(this_con);
                        size_t count = http_response_.to_encode(this_con.get_wb_ref());     // should I let user to do this ?TODO
                        LOG_DEBUG("encode %d", count);
                    } else {
                        SLOG_ERROR("no key of" << get<0>(key) << " " << get<1>(key));
                        NOTDONE();
                    }
                    http_request_.swap(HttpRequest());
                });
    }

    template <typename TCPClientClass>
    HttpClient<TCPClientClass>::~HttpClient() {}

    template <typename TCPServerClass>
    HttpServer<TCPServerClass>::~HttpServer() {}

template <typename TCPClientClass>
    HttpClient<TCPClientClass>& HttpClient<TCPClientClass>::set_httpresponse_msg_collector(X_SF&& cb){
        if (!msg_collector_&& !reflector_) {
            msg_collector_ = std::move(cb);
        }
    }

template <typename TCPClientClass>
    HttpClient<TCPClientClass>& HttpClient<TCPClientClass>::set_httpresponse_reflector(X_RF&& hre_ref){
        if (!msg_collector_ && !reflector_) {
            reflector_ = std::move(hre_ref);
        }
    }

template <typename TCPClientClass>
HttpClient<TCPClientClass>& HttpClient<TCPClientClass>::set_httprequest(HttpRequest&& http_request) {
    http_request_.swap(std::move(http_request));
    return *this;
}

template <typename TCPServerClass>
HttpServer<TCPServerClass>& HttpServer<TCPServerClass>::set_httpresponse(HttpResponse&& http_response) {
    http_response_.swap(std::move(http_response));
    return *this;
}

template <typename TCPServerClass>
HttpServer<TCPServerClass>& HttpServer<TCPServerClass>::set_action_of(string m_str, string uri, TCPCallBack&& cb) {
    auto found = map_.find(make_pair(m_str, uri));
    if (found == map_.end()) {
        map_[make_pair(m_str, uri)] = cb;
    } else {
        NOTDONE();
    }
    return *this;
}

//! @brief a wrapper for http_parser git submodule
//  TODO
//using ParserCallBack = std::function<void()>;
//using XParserCallBack = std::function<void(char const * at, size_t length)>;
//class HttpParser {
//public:
//    HttpParser ();
//    virtual ~HttpParser ();
//    void init_before_set_cb();
//    void set_cb(string cb_name, ParserCallBack&& cb);
//    void set_cb(string cb_name, XParserCallBack&& cb);
//    string get_some_in_cb();    //!< for user to invoke in callback, this method would check if caller is callback by is_in_cb()
//
//private:
//    bool is_in_cb();
//
//};

} /* my_http */

#endif /* ifndef HTTPPROTOCOL_H */
