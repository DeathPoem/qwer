#include "httpprotocol.h"

namespace my_http {
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

    size_t HttpRequest::to_encode(Buffer& buffer) {
        string content;
        content += Method2str(method_) + " " 
            + uri_ + " "
            + Version2str(version_)
            + CRLF_;
        for (auto v : headers_lines_) {
            content += get<0>(get<1>(v));
            content += ":";
            content += get<1>(get<1>(v));
            content += CRLF_;
        }
        if (!body_.empty()) {
            content += CRLF_;
            content += body_;
        }
        buffer.write_to_buffer(content.c_str(), content.size());
        return content.size();
    }

    size_t HttpRequest::to_decode(Buffer& buffer) {
        size_t size = buffer.get_readable_bytes();
        char * const content = new char[size + 1];
        memset(content, 0, size + 1);
        size_t parse_size = 0;
        {
            buffer.read_from_buffer(content, size);
            size_t cur = 0;
            size_t scaned = 0;
            size_t head_map_n = 0;
            while (cur < size) {
                if (content[cur] != '\r' 
                    && memcpy(&content[cur], &CRLF_, sizeof CRLF_) != 0
                    && !endone_) {
                    cur++;
                } else {
                    if (cur - scaned > 2048) {
                        NOTDONE();
                    }
                    string line = string(&content[scaned], cur - scaned);
                    if (version_ == HttpVersion::Invalid){
                        // init line
                        std::stringstream ss;
                        string m_str, v_str;
                        ss << line;
                        ss >> m_str;
                        ss >> uri_;
                        ss >> v_str;
                        method_ = str2Method(m_str);
                        version_ = str2Version(v_str);
                    } else if (version_ != HttpVersion::Invalid && memcpy(&content[scaned], "HTTP", 4) == 0) {
                        endone_ = true; // maybe two package in the buffer
                    } else if (noheaders_) {
                        // body
                        body_ = line;
                    } else if (cur != size - sizeof CRLF_ // not last CRLF_
                            && memcpy(&content[cur - sizeof CRLF_], &CRLF_, sizeof CRLF_) == 0) {
                        // blankline
                        noheaders_ = true;
                    } else {
                        // header
                        if (line.empty()) { throw std::runtime_error("can't be empty"); }
                        vector<string> header_tokens;
                        auto check = ::detail::splite_by_delimiter(line, header_tokens, string(":"));
                        assert(check == 1);
                        headers_lines_[head_map_n++] = make_pair(header_tokens[0], header_tokens[1]);
                    }
                    scaned = endone_ ? scaned : cur + sizeof CRLF_;
                } 
            }
            parse_size = scaned;
        }
        delete[] content;
        return parse_size;
    }

    size_t HttpResponse::to_encode(Buffer& buffer) {
        string content;
        content += Version2str(version_) + " "
            + Status2str(statuscode_)
            + CRLF_;
        for (auto v : headers_lines_) {
            content += get<0>(get<1>(v));
            content += ":";
            content += get<1>(get<1>(v));
            content += CRLF_;
        }
        if (!body_.empty()) {
            content += CRLF_;
            content += body_;
        }
        buffer.write_to_buffer(content.c_str(), content.size());
        return content.size();
    }

    size_t HttpResponse::to_decode(Buffer& buffer) {
        size_t size = buffer.get_readable_bytes();
        char * const content = new char[size + 1];
        memset(content, 0, size + 1);
        size_t parse_size = 0;
        {
            buffer.read_from_buffer(content, size);
            size_t cur = 0;
            size_t scaned = 0;
            size_t head_map_n = 0;
            while (cur < size) {
                if (content[cur] != '\r'
                    && memcpy(&content[cur], &CRLF_, sizeof CRLF_) != 0
                    && !endone_) {
                    cur++;
                } else {
                    if (cur - scaned > 2048) {
                        NOTDONE();
                    }
                    string line = string(&content[scaned], cur - scaned);
                    if (version_ == HttpVersion::Invalid && memcpy(&content[scaned], "HTTP", 4) == 0) {
                        // init line
                        std::stringstream ss;
                        string c_str, v_str;
                        ss << line;
                        ss >> v_str;
                        ss >> c_str;
                        c_str += " ";
                        ss >> c_str;
                        version_ = str2Version(v_str);
                        statuscode_ = str2Status(c_str);
                    } else if (version_ != HttpVersion::Invalid && memcpy(&content[scaned], "HTTP", 4) == 0) {
                        endone_ = true; // maybe two package in the buffer
                    } else if (noheaders_) {
                        // body
                        body_ = line;
                    } else if (cur != size - sizeof CRLF_ // not last CRLF_
                            && memcpy(&content[cur - sizeof CRLF_], &CRLF_, sizeof CRLF_) == 0) {
                        // blankline
                        noheaders_ = true;
                    } else {
                        // header
                        if (line.empty()) { throw std::runtime_error("can't be empty"); }
                        vector<string> header_tokens;
                        auto check = ::detail::splite_by_delimiter(line, header_tokens, string(":"));
                        assert(check == 1);
                        headers_lines_[head_map_n++] = make_pair(header_tokens[0], header_tokens[1]);
                    }
                    scaned = endone_ ? scaned : cur + sizeof CRLF_;
                } 
            }
            parse_size = scaned;
        }
        delete[] content;
        return parse_size;
    }

    void HttpRequest::get_test_default_one() {
        method_ = HttpMethod::Get;
        uri_ = "/hello.html";
        version_ = HttpVersion::Http1_1;
        assert(headers_lines_.empty());
        headers_lines_[0] = make_pair("User-Agent", " Mozilla/4.0 (compatible; MSIE5.01; Windows NT)");
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
        headers_lines_[2] = make_pair("Last-Modified", " Wed, 22 Jul 2009 19:15:56 GMT");
        headers_lines_[3] = make_pair("Content-Length", " 88");
        headers_lines_[4] = make_pair("Content-Type", " text/html");
        headers_lines_[5] = make_pair("Connection", " Closed");
        body_ = string("<html>") + HTML_NEWLINE_
            + string("<body>") + HTML_NEWLINE_
            + string("<h1>Hello, World!</h1>") + HTML_NEWLINE_
            + string("</body>") + HTML_NEWLINE_
            + string("</html>") + HTML_NEWLINE_;
    }
} /* my_http */ 
