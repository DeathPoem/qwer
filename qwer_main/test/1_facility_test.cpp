#include "facility.h"
#include "gtest/gtest.h"

TEST(test_case_1, test_0_noname) {
    EXPECT_EQ(0, my_http::MyFacilityTest());
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
    LOG_ERROR("fuck!");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
