#include "facility.h"
#include "gtest/gtest.h"

TEST(some_test_case_name, some_test_name) {
    EXPECT_EQ(0, my_http::MyFacilityTest());
}
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
