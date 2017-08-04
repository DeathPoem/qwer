//#include "../../qwer_main/src/lib/facility.h"
#include "facility.h"
#include "event.h"
#include "httpprotocol.h"

using namespace my_http;
int main() {
    LOG_SET_FILE_P("./tmp_log.txt", true);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_multithread");
    EventManagerWrapper emw;
    HttpResponse x_http_response;
    x_http_response.version_ = HttpMsg::HttpVersion ::Http1_1;
    x_http_response.statuscode_ = HttpMsg::HttpStatusCodes::c_200;
    x_http_response.headers_lines_[0] = make_pair("Connection", " close");
    x_http_response.body_ = string("<html><body>") +
            string("<h1>Hello, World!</h1>") +
            string("</body></html>") ;
    HttpServer<TCPServer> httpserver(&emw,
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8080));
    httpserver.set_action_of("GET", "/hello.html", [&httpserver](TCPConnection& this_con){
                LOG_DEBUG("server : receive GET request");
            }).set_httpresponse(std::move(x_http_response));
    cout << "begin server" << endl;
    for (int i = 0; i < 3600; ++i) {
        if (i % 60 == 0) {
            cout << "i = " << i << endl;
        }
        emw.loop_once(200);
    }
    cout << "end of server" << endl;
}
