#include "httpprotocol.h"

namespace my_http {

    HttpMsg::~HttpMsg() {}

    HttpMsg::HttpMsg() {}

HttpMsg::HttpMethod HttpMsg::str2Method(string s) {
    if (s == "Get") {
        return HttpMethod::Get;
    } else {
        NOTDONE();
    }
}

HttpMsg::HttpVersion HttpMsg::str2Version(string s) {
    if (s == "HTTP/1.1") {
        return HttpVersion::Http1_1;
    } else if (s == "HTTP/1.0") {
        return HttpVersion::Http1_0;
    } else {
        NOTDONE();
    }
}

HttpMsg::HttpStatusCodes HttpMsg::str2Status(string s) {
    if (s == "404 Not Found") {
        return HttpStatusCodes::c_404;
    } else if (s == "200 OK") {
        return HttpStatusCodes::c_200;
    } else {
        NOTDONE();
    }
}

string HttpMsg::Method2str(HttpMethod hm) {
    if (hm == HttpMethod::Get) {
        return string("Get");
    } else {
        NOTDONE();
    }
}

string HttpMsg::Version2str(HttpVersion hv) {
    if (hv == HttpVersion::Http1_1) {
        return string("HTTP/1.1");
    } else if (hv == HttpVersion::Http1_0) {
        return string("HTTP/1.0");
    } else {
        NOTDONE();
    }
}

string HttpMsg::Status2str(HttpStatusCodes hs) {
    if (hs == HttpStatusCodes::c_404) {
        return string("404 Not Found");
    } else if (hs == HttpStatusCodes::c_200) {
        return string("200 OK");
    } else {
        NOTDONE();
    }
}

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

    HttpResponse::HttpResponse() {}

    HttpResponse::~HttpResponse() {}

    HttpClient::~HttpClient() {}

    HttpServer::~HttpServer() {}

size_t HttpRequest::to_encode(Buffer& buffer) {
    string content;
    content +=
        Method2str(method_) + " " + uri_ + " " + Version2str(version_) + CRLF_;
    for (auto v : headers_lines_) {
        content += get<0>(get<1>(v));
        content += ":";
        content += get<1>(get<1>(v));
        content += CRLF_;
    }
    if (!body_.empty()) {
        content += CRLF_;
        content += body_;
        content += CRLF_;
    }
    buffer.write_to_buffer(content.c_str(), content.size());
    return content.size();
}

size_t HttpMsg::try_decode_of(Buffer& buffer,
                              std::function<void(string)> const& func,
                              map<int, pair<string, string>>& headers_lines_ref,
                              HttpVersion& version_ref, string& body_ref) {
    size_t const size = buffer.get_readable_bytes();
    char* content = new char[size + 1];
    memset(content, 0, size + 1);
    size_t parse_size = 0;
    {
        buffer.read_from_buffer(content, size);
        size_t cur = 0;
        size_t scaned = 0;
        size_t head_map_n = 0;
        bool noheaders = false;
        bool endone = false;
        assert(*(content + size - 1) != '\r');
        while (cur < size) {
            if (cur < size - 1 && ::memcmp(content + cur, CRLF_.c_str(), CRLF_.size()) == 0) {
                if (cur - scaned > 2048) {
                    NOTDONE();
                }
                string line = string(content + scaned, cur - scaned);
                if (version_ref == HttpVersion::Invalid) {
                    // init line
                    func(line);
                } else if (version_ref != HttpVersion::Invalid &&
                           ::memcmp(content + scaned, "HTTP", 4) == 0) {
                    endone = true;  // maybe two package in the buffer
                } else if (noheaders) {
                    // body
                    body_ref = line;
                } else if (!noheaders && cur < size - CRLF_.size()  // not last CRLF_
                           && line == "") { // double CRLF would cause this
                    // blankline
                    noheaders = true;
                } else {
                    // header
                    if (line.empty()) { ABORT("can't be empty"); }
                    vector<string> header_tokens;
                    int check;
                    check = detail::splite_by_delimiter(
                                            line, header_tokens, string(":"));
                    assert(check >= 1);
                    if (check == 1) {
                        headers_lines_ref[head_map_n++] =
                                make_pair(header_tokens[0], header_tokens[1]);
                    } else {
                        string rest = header_tokens[1];
                        for (int i = 1; i < check; i++) {
                            rest += ":";
                            rest += header_tokens[i + 1];
                        }
                        headers_lines_ref[head_map_n++] = make_pair(header_tokens[0], rest);
                    }
                }
                scaned = endone ? scaned : cur + CRLF_.size();
            } else {
                if (endone) { break; }
            }
            cur++;
        }
        parse_size = scaned;
    }
    delete[] content;
    return parse_size;
}

size_t HttpRequest::to_decode(Buffer& buffer) {
    auto a_func = [this](string line) {
        // init line
        std::stringstream ss;
        string m_str, v_str;
        ss << line;
        ss >> m_str;
        ss >> uri_;
        ss >> v_str;
        method_ = str2Method(m_str);
        version_ = str2Version(v_str);
        return;
    };
    auto check = try_decode_of(buffer, a_func, headers_lines_, version_, body_);
    if (check < 8) {
        NOTDONE();
    }
    return check;
}

size_t HttpResponse::to_encode(Buffer& buffer) {
    string content;
    content += Version2str(version_) + " " + Status2str(statuscode_) + CRLF_;
    for (auto v : headers_lines_) {
        content += get<0>(get<1>(v));
        content += ":";
        content += get<1>(get<1>(v));
        content += CRLF_;
    }
    if (!body_.empty()) {
        content += CRLF_;
        content += body_;
        content += CRLF_;
    }
    buffer.write_to_buffer(content.c_str(), content.size());
    return content.size();
}

size_t HttpResponse::to_decode(Buffer& buffer) {
    auto a_func = [this](string line) {
        // init line
        std::stringstream ss;
        string c_str1, v_str, c_str2;
        ss << line;
        ss >> v_str;
        ss >> c_str1;
        ss >> c_str2;
        version_ = str2Version(v_str);
        statuscode_ = str2Status(c_str1 + " " + c_str2);
        return;
    };
    auto check = try_decode_of(buffer, a_func, headers_lines_, version_, body_);
    if (check < 8) {
        NOTDONE();
    }
    return check;
}

void HttpRequest::get_test_default_one() {
    method_ = HttpMethod::Get;
    uri_ = "/hello.html";
    version_ = HttpVersion::Http1_1;
    assert(headers_lines_.empty());
    headers_lines_[0] = make_pair(
        "User-Agent", " Mozilla/4.0 (compatible; MSIE5.01; Windows NT)");
    headers_lines_[1] = make_pair("Host", " www.tutorialspoint.com");
    headers_lines_[2] = make_pair("Accept-Language", " en-us");
    headers_lines_[3] = make_pair("Accept-Encoding", " gzip, deflate");
    headers_lines_[4] = make_pair("Connection", " Keep-Alive");
}

void HttpResponse::get_test_default_one() {
    version_ = HttpVersion::Http1_1;
    statuscode_ = HttpStatusCodes::c_200;
    assert(headers_lines_.empty());
    headers_lines_[0] = make_pair("Date", " Mon, 27 Jul 2009 12:28:53 GMT");
    headers_lines_[1] = make_pair("Server", " Apache/2.2.14 (Win32)");
    headers_lines_[2] =
        make_pair("Last-Modified", " Wed, 22 Jul 2009 19:15:56 GMT");
    headers_lines_[3] = make_pair("Content-Length", " 88");
    headers_lines_[4] = make_pair("Content-Type", " text/html");
    headers_lines_[5] = make_pair("Connection", " Closed");
    body_ = string("<html>") + HTML_NEWLINE_ + string("<body>") +
            HTML_NEWLINE_ + string("<h1>Hello, World!</h1>") + HTML_NEWLINE_ +
            string("</body>") + HTML_NEWLINE_ + string("</html>") +
            HTML_NEWLINE_;
}

HttpServer::HttpServer(EventManagerWrapper* emwp, Ipv4Addr listen_ip)
    : tcp_server_(emwp, listen_ip) {
        tcp_server_.set_msg_callback([this](uint32_t seqno){
                    LOG_DEBUG("httpserver msg callback");
                    auto this_con = tcp_server_.get_shared_tcpcon_ref_by_seqno(seqno);
                    this_con->get_rb_ref().consume(
                            http_request_.to_decode(this_con->get_rb_ref()));
                    pair<string, string> key = make_pair(
                            http_request_.get_method_str(),
                            http_request_.uri_);
                    assert(!map_.empty());
                    auto found = map_.find(key);
                    if (found != map_.end()) {
                        auto& func_ref = map_[key];
                        func_ref(*this_con);
                        http_response_.to_encode(this_con->get_wb_ref());
                    } else {
                        NOTDONE();
                    }
                    http_request_.swap(HttpRequest());
                });
    }

string HttpRequest::get_method_str() {
    return Method2str(method_);
}

HttpClient::HttpClient(EventManagerWrapper* emwp, Ipv4Addr local_ip,
                       Ipv4Addr peer_ip)
    : tcp_client_(emwp, peer_ip, local_ip) {
    tcp_client_
        .set_tcpcon_after_connected_callback([this](TCPConnection& this_con) {
            LOG_DEBUG("httpclient connect callback");
            auto& wb = this_con.get_wb_ref();
            auto check = http_request_.to_encode(wb);
            auto check1 = this_con.to_write();
            if (check != check1 || check == 0) { NOTDONE(); }
        })
        .set_msg_callback([this](uint32_t seqno) {
            if (msg_collector_ && !reflector_) {
                LOG_DEBUG("httpclient msg callback");
                auto this_con = tcp_client_.get_shared_tcpcon_ref();
                this_con->get_rb_ref().consume(
                    http_response_.to_decode(this_con->get_rb_ref()));
                msg_collector_(http_response_.body_);
            } else if (reflector_ && !msg_collector_) {
                NOTDONE();
            } else {
                NOTDONE();
            }
        });
}

    HttpClient& HttpClient::set_httpresponse_msg_collector(X_SF&& cb){
        if (!msg_collector_&& !reflector_) {
            msg_collector_ = std::move(cb);
        }
    }

    HttpClient& HttpClient::set_httpresponse_reflector(X_RF&& hre_ref){
        if (!msg_collector_ && !reflector_) {
            reflector_ = std::move(hre_ref);
        }
    }

void HttpRequest::swap(HttpRequest&& other) {
    uri_.swap(other.uri_);
    std::swap(version_, other.version_);
    std::swap(method_, other.method_);
    headers_lines_.swap(other.headers_lines_);
    body_.swap(other.body_);
}

    void HttpResponse::swap(HttpResponse &&other) {
        body_.swap(other.body_);
        std::swap(version_, other.version_);
        std::swap(statuscode_, other.statuscode_);
        headers_lines_.swap(other.headers_lines_);
    }

HttpClient& HttpClient::set_httprequest(HttpRequest&& http_request) {
    http_request_.swap(std::move(http_request));
    return *this;
}

HttpServer& HttpServer::set_httpresponse(HttpResponse&& http_response) {
    http_response_.swap(std::move(http_response));
    return *this;
}

HttpServer& HttpServer::set_action_of(string m_str, string uri, TCPCallBack&& cb) {
    auto found = map_.find(make_pair(m_str, uri));
    if (found == map_.end()) {
        map_[make_pair(m_str, uri)] = cb;
    } else {
        NOTDONE();
    }
    return *this;
}
} /* my_http */
