#include "facility.h"
#include "gtest/gtest.h"

TEST(test_case_0, test_0_noname) {
    EXPECT_EQ(0, my_http::MyFacilityTest());
}

TEST(test_case_0, test_1_log) {
    using my_http::Logger;
    Logger::get_logger().repare().log("for log testing thing %d", 1);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
