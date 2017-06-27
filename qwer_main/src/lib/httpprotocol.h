#ifndef HTTPPROTOCOL_H
#define HTTPPROTOCOL_H value

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
    enum class HttpMethod {Invalid, Get};
    enum class HttpVersion {Invalid, Http1_0, Http1_1};
    enum class HttpStatusCodes {Invalid, c_200, c_404};
    HttpMethod str2Method(string s);
    string Method2str(HttpMethod hm);
    HttpVersion str2Version(string s);
    string Version2str(HttpVersion hv);
    HttpStatusCodes str2Status(string s);
    string Status2str(HttpStatusCodes hs);
    const string CRLF_ = "\r\n";
    const string HTML_NEWLINE_ = string(u8"\u8629");
};

class HttpRequest : public HttpMsg {
public:
    HttpRequest();
    virtual ~HttpRequest();
    void get_test_default_one();
    //GET /hello.html HTTP/1.1
    //User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
    //Host: www.tutorialspoint.com
    //Accept-Language: en-us
    //Accept-Encoding: gzip, deflate
    //Connection: Keep-Alive
    virtual size_t to_decode(Buffer& buffer) override;
    virtual size_t to_encode(Buffer& buffer) override;
    
private:
    string uri_;
    HttpVersion version_ = HttpVersion::Invalid;
    HttpMethod method_;
    map<int, pair<string, string>> headers_lines_;    // headers can't be empty
    bool noheaders_ = false, endone_ = false;
    string body_;
};

class HttpResponse : public HttpMsg {
public:
    HttpResponse();
    virtual ~HttpResponse();
    void get_test_default_one();
    //HTTP/1.1 200 OK
    //Date: Mon, 27 Jul 2009 12:28:53 GMT
    //Server: Apache/2.2.14 (Win32)
    //Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
    //Content-Length: 88
    //Content-Type: text/html
    //Connection: Closed
    //<html>
    //   <body>
    //
    //      <h1>Hello, World!</h1>
    //
    //         </body>
    //         </html>

    virtual size_t to_decode(Buffer& buffer) override;
    virtual size_t to_encode(Buffer& buffer) override;
private:
    HttpVersion version_ = HttpVersion::Invalid;
    HttpStatusCodes statuscode_;
    map<int, pair<string, string>> headers_lines_;// headers can't be empty
    bool noheaders_ = false, endone_ = false;
    string body_;
};
} /* my_http */

namespace detail {
    template<typename ContainerType>
        int splite_by_delimiter(ContainerType const & origin, std::vector<ContainerType> & to_fill_vec, ContainerType const & delimiter) {
            // do nothing
            throw std::runtime_error("this template only support string");
        }

    using std::string;
    template<>
        int splite_by_delimiter(string const & origin, std::vector<string> & to_fill_vec, string const & delimiter) {
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
                to_fill_vec.push_back(origin.substr(pos1 - origin.begin(), pos2 - pos1));
                pos1 = f;
            }
            pos2 = origin.end();
            to_fill_vec.push_back(origin.substr(pos1 - origin.begin(), pos2 - pos1));
            return founds.size();
        }
} /* detail  */ 
#endif /* ifndef HTTPPROTOCOL_H */
