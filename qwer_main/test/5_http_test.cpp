#include "facility.h"
#include "httpprotocol.h"
#include "gtest/gtest.h"

using namespace my_http;

void x_client_thread() {
    EventManagerWrapper emw;
    string outer;
    HttpRequest x_http_request;
    x_http_request.get_test_default_one();
    HttpClient httpclient(&emw, 
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 9214),
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 80));
    httpclient.set_httprequest(std::move(x_http_request))
        .set_httpresponse_msg_collector([&outer](string const & str) {
                outer = str;
            });
    for (int i = 0; i < 10; ++i) {
        emw.loop_once(500);
    }
    EXPECT_EQ(outer, "hello");
}

void x_server_thread() {
    EventManagerWrapper emw;
    HttpResponse x_http_response;
    x_http_response.get_test_default_one();
    HttpServer httpserver(&emw, 
            Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 80));
    httpserver.set_action_of("Get", "", [&httpserver](TCPConnection& this_con){
                auto& wb = this_con.get_wb_ref();
                httpserver.to_encode(wb);
            }).set_httpresponse(std::move(x_http_response));
    for (int i = 0; i < 10; ++i) {
        emw.loop_once(500);
    }
}

TEST(test_case_5, test_http) {
    LOG_SET_FILE_P("", true);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_http");
    auto client_thread = std::thread(x_client_thread);
    auto server_thread = std::thread(x_server_thread);
    client_thread.join();
    server_thread.join();
}

TEST(test_case_5, test_http_in_threadpool) {

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
