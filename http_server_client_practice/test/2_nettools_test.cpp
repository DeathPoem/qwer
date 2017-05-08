#include "nettools.h"
#include "facility.h"
#include "gtest/gtest.h"

TEST(test_case_2, test_ipv4addr_constructor) {
    using my_http::Ipv4Addr;
    Ipv4Addr test_ip = Ipv4Addr("localhost", 1234);
    std::cout << test_ip.get_ip_str() << std::endl;
    std::string ip_s_0 = Ipv4Addr::host2ip_str("localhost");
    std::string ip_s_1 = test_ip.get_ip_str();
    EXPECT_EQ(ip_s_0, ip_s_1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
