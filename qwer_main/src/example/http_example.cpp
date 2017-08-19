#include "facility.h"
#include "httpprotocol.h"
#include "webservice.h"

using namespace my_http;

//! @brief single thread server
int deamon_0() {
    LOG_SET_FILE_P("./tmp_log.txt", false);
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
                this_con.to_lazy_close();
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

//! @brief multi thread server
int deamon_1() {
    // set log
    LOG_SET_FILE_P("./tmp_log.txt", false);
    LOG_SET_LEVEL("WARN");
    LOG_DEBUG(" \n \n in test_multithread");
    // 
    // set http api
    shared_ptr<HttpResponse> x_http_response(new HttpResponse());
    x_http_response->version_ = HttpMsg::HttpVersion ::Http1_1;
    x_http_response->statuscode_ = HttpMsg::HttpStatusCodes::c_200;
    x_http_response->headers_lines_[0] = make_pair("Connection", " close");
    x_http_response->body_ = string("<html><body>") +
            string("<h1>Hello, World!</h1>") +
            string("</body></html>") ;  //!< you can also use FileReader("filename").to_string();
    shared_ptr<HttpSetting> httpsettings(new HttpSetting());
    HttpServer<MultiServer> httpserver(100,
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8080),
            httpsettings);
    httpsettings->cash_response_of("GET", "/hello.html", move(x_http_response))
            .set_action_of("GET", "/hello.html", [&httpserver]
                    (TCPConnection& this_con, shared_ptr<HttpRequest> req, shared_ptr<HttpResponse> res){ //!< you don't need to modify this res, because res is cashed, (effective consider)
                //! you can add your header handler and filters here like Nginx provide
                // your_header_handler(req->headers_lines_);
                this_con.to_lazy_close();   //!< close this connection after write the content, because your response headers_lines_[0]
            });
    // serve and block until functor return true;
    cout << "begin server" << endl;
    httpserver.Start([](){
                std::this_thread::sleep_for(1000000ms);   //!< sleep of this command thread won't affect threads in pool
                return false;
            });
    cout << "end of server" << endl;
    httpserver.Exit();
}

//! @brief multi thread server with middlewares and DBholder
int deamon_2() {
    LOG_SET_FILE_P("./tmp_log.txt", false);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_multithread");
    shared_ptr<HttpResponse> x_http_response(new HttpResponse());
    shared_ptr<HttpSetting> httpsettings(new HttpSetting());
    HttpServer<MultiServer> httpserver(100,
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8080),
            httpsettings);

    // DBholder is a very simple SQLiteCpp wrapper
    DBholder mydb;
    httpsettings->use([&mydb](TCPConnection& this_con, shared_ptr<HttpRequest> req, shared_ptr<HttpResponse> res, NextCallBack next) {
                mydb.SingleQuery("SELECT value FROM test WHERE id=2");
                next(); //!< don't invoke next would cause other MiddleWareCb unable to be invoked
            });
    // serve and block until functor return true;
    cout << "begin server" << endl;
    httpserver.Start([](){
                std::this_thread::sleep_for(10000ms);   //!< sleep of this command thread won't affect threads in pool
                return true;
            });
    cout << "end of server" << endl;
    httpserver.Exit();
}

int main() {
    return deamon_1();
}
