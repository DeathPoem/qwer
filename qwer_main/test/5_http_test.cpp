#include "facility.h"
#include "gtest/gtest.h"

using namespace my_http;
TEST(test_case_5, test_http) {
    string ss = "ss";
    EXPECT_EQ("ss", ss);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
