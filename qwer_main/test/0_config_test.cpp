#include "gtest/gtest.h"
#include "json.hpp"

static int static_func() {
    return 1;
}

TEST(test_case_0, gtest_config_test) { EXPECT_EQ(1, static_func()); }

TEST(test_case_0, json_config_test) {
    using json = nlohmann::json;
    json j2 = {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {
                       {"everything", 42}

                   }},
        {"list", {1, 0, 2}},
        {"object", {
                       {"currency", "USD"},
                       {"value", 42.99}

                   }}

    };
    ASSERT_NE(j2, nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
