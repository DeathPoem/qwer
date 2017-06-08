#include "facility.h"
#include "gtest/gtest.h"
#include "nettools.h"
#include "threadpool.h"

TEST(test_case_2, test_ipv4addr_constructor) {
    using my_http::Ipv4Addr;
    Ipv4Addr test_ip = Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 1234);
    std::string ip_s_0 = Ipv4Addr::host2ip_str("localhost");
    std::string ip_s_1 = test_ip.get_ip_str();
    EXPECT_EQ(ip_s_0, ip_s_1);
}

TEST(test_case_2, test_buffer) {
    using my_http::Buffer;
    char one_buffer[100];
    char two_buffer[100];
    memset(one_buffer, '\0', sizeof(one_buffer));
    memset(two_buffer, '\0', sizeof(two_buffer));
    sprintf(one_buffer, "this is something for test");
    Buffer buf;
    ASSERT_EQ(strlen(one_buffer), 26);
    buf.write_to_buffer(one_buffer, strlen(one_buffer));
    try {
        buf.read_from_buffer(two_buffer, buf.get_readable_bytes());
    } catch (std::runtime_error& e) {
        std::cerr << "Error:" << e.what();
    }
    EXPECT_STREQ(one_buffer, two_buffer);
}

TEST(test_case_2, test_thread_pool) {
    using my_http::ThreadPool;
    using my_http::Logger;
    using std::chrono_literals::operator""ms;
    auto hc = std::thread::hardware_concurrency();
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    LOG_INFO("hardware_concurrency() get hc = %d", hc);
    ThreadPool threadpool_ob(hc + 1, 1000);
    threadpool_ob.add_task([]() {
        std::this_thread::sleep_for(2ms);
        LOG_INFO("in thread task1");
    });
    threadpool_ob.add_task([]() {
        std::this_thread::sleep_for(3ms);
        LOG_INFO("in thread task2");
    });
    std::string outer = "awe?";
    threadpool_ob.add_task([&outer]() {
        std::this_thread::sleep_for(3ms);
        outer = "awesome!";
        LOG_INFO("in thread task3");
    });
    threadpool_ob.start();
    LOG_INFO("before threadpool_ob stop()");
    threadpool_ob.stop();
    EXPECT_EQ(outer, "awesome!");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
