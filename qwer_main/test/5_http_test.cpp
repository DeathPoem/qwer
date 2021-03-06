#include "facility.h"
#include "httpprotocol.h"
#include "gtest/gtest.h"

using namespace my_http;

static atomic<bool> server_to_exit(false);
static atomic<bool> server_is_on(false);
static mutex x_mm;
static condition_variable x_cv;

void x_client_thread(int x_port) {
    EventManagerWrapper emw;
    string outer;
    HttpRequest x_http_request;
    x_http_request.get_test_default_one();
    HttpClient<TCPClient> httpclient(&emw, 
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), x_port),
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8000));
    httpclient.set_httprequest(std::move(x_http_request))
        .set_httpresponse_msg_collector([&outer](string const & str) {
                outer = str;
            });
    for (int i = 0; i < 20; ++i) {
        emw.loop_once(200);
    }
    EXPECT_EQ(outer, "<html>\xE8\x98\xA9<body>\xE8\x98\xA9<h1>Hello, World!</h1>\xE8\x98\xA9</body>\xE8\x98\xA9</html>\xE8\x98\xA9");
}

void x_server_thread() {
    EventManagerWrapper emw;
    HttpResponse x_http_response;
    x_http_response.get_test_default_one();
    HttpServer<TCPServer> httpserver(&emw,
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8000));
    httpserver.set_action_of("GET", "/hello.html", [&httpserver](TCPConnection& this_con){
                LOG_DEBUG("server : receive Get request");
            }).set_httpresponse(std::move(x_http_response));
    for (int i = 0; i < 100; ++i) {
        if (!server_to_exit) {
            server_is_on = true;
            x_cv.notify_one();
            emw.loop_once(500);
        }
    }
}

void y_server_thread() {
    // set http api
    shared_ptr<HttpResponse> x_http_response(new HttpResponse());
    x_http_response->get_test_default_one();
    shared_ptr<HttpSetting> httpsettings(new HttpSetting());
    HttpServer<MultiServer> httpserver(100,
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8000),
            httpsettings);
    httpsettings->cash_response_of("GET", "/hello.html", move(x_http_response))
            .set_action_of("GET", "/hello.html", [&httpserver]
                    (TCPConnection& this_con, shared_ptr<HttpRequest> req, shared_ptr<HttpResponse> res){
                this_con.to_lazy_close();
            });
    // serve and block until functor return true;
    httpserver.Start([](){ return true; });
    server_is_on = true;
    x_cv.notify_one();
    {
        std::unique_lock<mutex> lk(x_mm);
        x_cv.wait(lk, [](){ return server_to_exit == true;});
    }
    httpserver.Exit();
}

TEST(test_case_5, test_http) {
    LOG_SET_FILE_P("", false);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_http");
    auto server_thread = std::thread(x_server_thread);
    {
        std::unique_lock<mutex> lk(x_mm);
        x_cv.wait(lk, [](){ return server_is_on == true;});
    }
    auto client_thread = std::thread(std::bind(x_client_thread, 12398));
    client_thread.join();
    LOG_DEBUG("client threadd join");
    server_to_exit = true;
    server_thread.join();
    LOG_DEBUG("server threadd join");
    server_to_exit = false;
    server_is_on = false;
}

TEST(test_case_5, test_http_in_threadpool) {
    LOG_SET_FILE_P("", false);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_http");
    auto server_thread = std::thread(y_server_thread);
    {
        std::unique_lock<mutex> lk(x_mm);
        x_cv.wait(lk, [](){ return server_is_on == true;});
    }
    auto client_thread = std::thread(std::bind(x_client_thread, 14898));
    client_thread.join();
    LOG_DEBUG("client threadd join");
    server_to_exit = true;
    x_cv.notify_one();
    server_thread.join();
    LOG_DEBUG("server threadd join");
    server_to_exit = false;
    server_is_on = false;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
