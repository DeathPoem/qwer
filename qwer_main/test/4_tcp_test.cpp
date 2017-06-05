#include "gtest/gtest.h"
#include "facility.h"
#include "event.h"
#include "nettools.h"
#include <thread>
#include <string.h>
#include "tcpserver.h"
#include <stdio.h>
#include <mutex>
#include <chrono>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <condition_variable>
#include <sys/ioctl.h>
#include <sys/socket.h>

using namespace my_http;


std::condition_variable cv;
std::mutex m;
static int state = 0;
// connecting and send some
void clientfunc(int port, Ipv4Addr ipaddr) {
    Ipv4Addr ip(Ipv4Addr::host2ip_str("localhost"), port);
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        NOTDONE();
    }
    ip.ip_bind_socketfd(socketfd);
    struct sockaddr_in peer = ipaddr.get_socketaddr_in();
    unsigned int peerlen = sizeof(peer);

    int check = -1;
    char buf[99];
    char buff[99];
    {
        std::unique_lock<std::mutex> lk(m);
        LOG_DEBUG("block until accept!");
        cv.wait(lk, [](){return state == 1;});
        state += 1;
        using namespace std::chrono_literals;
        while (check != 0) {
            std::this_thread::sleep_for(1s);        
            check = ::connect(socketfd, (sockaddr *)&peer, peerlen);
            if (check != 0) { LOG_INFO("connect , errno =", strerror((errno))); }
        }
        LOG_DEBUG("after connect!");
        ::sprintf(buf, "hello? world!");
        check = ::write(socketfd, buf, strlen(buf));

        SLOG_DEBUG("after client write! check =" << check << ",buflen=" << ::strlen(buf));
        assert(check == ::strlen(buf));
        if (check < 0) {
            NOTDONE();
        }
    }
    cv.notify_one();
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []() { return state == 3; });
        LOG_DEBUG("receive from server");
        check = ::read(socketfd, buff, ::strlen(buf));
        ::close(socketfd);
        EXPECT_STREQ(buff, buf);
    }
}

void echo(int fd) {
    char buf[999];
    int bytesAvailable;
    ioctl(fd, FIONREAD, &bytesAvailable);
    int n = ::read(fd, buf, bytesAvailable);
    if (bytesAvailable > 0 && n == bytesAvailable) {
        SLOG_INFO(" by =" << bytesAvailable << " n =" << n << "read is=" << buf);
    } else {
        SLOG_ERROR(" by =" << bytesAvailable << " n =" << n);
        assert(bytesAvailable > 0 && n == bytesAvailable);
    }
    ::write(fd, buf, n);
    ::close(fd);
}

TEST(test_case_4, test_raw_tcp) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");

    Ipv4Addr ip(Ipv4Addr::host2ip_str("localhost"), 9876);
    LOG_DEBUG("start client thread");

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        NOTDONE();
    }
    ip.ip_bind_socketfd(socketfd);

    ::listen(socketfd, SOMAXCONN);

    auto client_thread = std::thread([&ip](){ clientfunc(51912, ip); });
    LOG_DEBUG("listen");
    struct sockaddr peer;
    unsigned int peerlen = sizeof(peer);
    int new_fd = -1;
    for (int i : {1}) {
        LOG_DEBUG("f");
        {
            std::lock_guard<std::mutex> lk(m);
            state += 1;
            // not 100 percent thread safe
        }
        new_fd = ::accept(socketfd, &peer, &peerlen);
        {
            std::lock_guard<std::mutex> lk(m);
            SLOG_DEBUG("after accept" << "state =" << state);
        }
        if (new_fd < 0) { NOTDONE(); }
        cv.notify_one();
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [](){return state == 2;});
            state += 1;
            echo(new_fd);
            LOG_DEBUG("after echo");
        }
    }
    client_thread.join();
    ::close(socketfd);
}

TEST(test_case_4, test_tcp_acceptor) {
    //LOG_SET_FILE("");
    //LOG_SET_LEVEL("INFO");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
