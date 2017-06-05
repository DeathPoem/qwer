#include "facility.h"
#include <thread>
#include <sstream>
#include "gtest/gtest.h"

TEST(test_case_1, test_0_noname) {
    EXPECT_EQ(0, my_http::MyFacilityTest());
}

TEST(test_case_1, test_0_stream) {
    using my_http::Logger;
    LOG_SET_FILE("");
    LOG_SET_LEVEL("DEBUG");
    int i = 0;
    SLOG_ERROR("fuck" << i);
}

TEST(test_case_1, test_1_log) {
    using my_http::Logger;
    Logger::get_logger().repare("").log("for log testing thing %d", 1);
}

TEST(test_case_1, test_1_log_v) {
    using my_http::Logger;
    int a = 2;
    LOG_SET_FILE("");
    Logger::get_logger().log_v(1, __FILE__, my_http::to_string(__LINE__).c_str(), __TIME__, "fuck you %d times!", a); \
}

TEST(test_case_1, test_1_log_level) {
    using my_http::Logger;
    LOG_SET_FILE("");
    LOG_SET_LEVEL("DEBUG");
    LOG_ERROR("\nfuck!");
}

thread_local int thrlocal_int = 0;

void thread_func() {
    thrlocal_int = 1;
    my_http::cout << "other,int = " << thrlocal_int << my_http::endl;
}

TEST(test_case_1, test_raw_thread_local) {
    thrlocal_int = 2;
    std::thread t0(thread_func);
    my_http::cout << "main,int = " << thrlocal_int << my_http::endl;
    t0.join();
    SUCCEED();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
