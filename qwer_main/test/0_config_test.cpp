#include "gtest/gtest.h"
#include "config.h"
#include "json.hpp"
#include "SQLiteCpp/SQLiteCpp.h"

static int static_func() {
    return 1;
}

TEST(test_case_0, gtest_config_test) { EXPECT_EQ(1, static_func()); }   //!< gtest

TEST(test_case_0, json_config_test) {   //!< json
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

TEST(test_case_0, sqlite_test) {
    std::string dbpath = std::string() + GET_MY_COMPILE_ROOT_PATH + "/stuff/defaultdb";
    SQLite::Database db(dbpath, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
