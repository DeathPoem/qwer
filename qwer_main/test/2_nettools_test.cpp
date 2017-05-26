#include "nettools.h"
#include "facility.h"
#include "gtest/gtest.h"

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
