#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include "event.h"
#include "facility.h"
#include "gtest/gtest.h"
#include "nettools.h"
#include "tcpserver.h"

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
        cv.wait(lk, []() { return state == 1; });
        state += 1;
        using namespace std::chrono_literals;
        while (check != 0) {
            std::this_thread::sleep_for(1s);
            check = ::connect(socketfd, (sockaddr*)&peer, peerlen);
            if (check != 0) {
                LOG_INFO("connect , errno =", strerror((errno)));
            }
        }
        LOG_DEBUG("after connect!");
        ::sprintf(buf, "hello? world!");
        check = ::write(socketfd, buf, strlen(buf));

        SLOG_DEBUG("after client write! check =" << check << ",buflen="
                                                 << ::strlen(buf));
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
    ::ioctl(fd, FIONREAD, &bytesAvailable);
    int n = ::read(fd, buf, bytesAvailable);
    if (bytesAvailable > 0 && n == bytesAvailable) {
        SLOG_INFO(" by =" << bytesAvailable << " n =" << n
                          << "read is=" << buf);
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

    int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        NOTDONE();
    }
    ip.ip_bind_socketfd(socketfd);

    ::listen(socketfd, SOMAXCONN);

    auto client_thread = std::thread([&ip]() { clientfunc(51912, ip); });
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
            SLOG_DEBUG("after accept"
                       << "state =" << state);
        }
        if (new_fd < 0) {
            NOTDONE();
        }
        cv.notify_one();
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []() { return state == 2; });
            state += 1;
            echo(new_fd);
            LOG_DEBUG("after echo");
        }
    }
    client_thread.join();
    ::close(socketfd);
}

struct TCPConnectionHolder {
    vector<shared_ptr<TCPConnection>> shared_p_tc_vec_;
};

struct XMsgResponser {
    void do_it(Buffer& rb, Buffer& wb) {
        char readto[rb.get_readable_bytes()];
        rb.read_from_buffer(readto, rb.get_readable_bytes());
        auto consume_size = strlen(readto);
        rb.consume(consume_size);
        char* result = readto;
        wb.write_to_buffer(result, strlen(result));
    }
    void do_it(char* msg, char* after) { ::memcpy(msg, after, strlen(msg)); }
    void client_do_it(Buffer& rb, Buffer& wb) {}
};

TEST(test_case_4, test_tcp_acceptor_connector) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    EventManagerWrapper emw;
    Acceptor acceptor(&emw);
    Ipv4Addr listen_ip(Ipv4Addr::host2ip_str("localhost"), 9876);
    TCPConnectionHolder holder;
    TCPConnectionHolder clientholder;
    size_t app_size = 100;
    XMsgResponser msg_responser;
    Ipv4Addr clientip(Ipv4Addr::host2ip_str("localhost"), 87211);
    Connector my_connector(&emw);
    acceptor.set_listen_addr(listen_ip)
        .set_accept_readable_callback([&holder, &app_size, &msg_responser](
            shared_ptr<TCPConnection> shared_p_tc) {
            holder.shared_p_tc_vec_.push_back(shared_p_tc);
            shared_p_tc
                ->set_normal_readable_callback(
                    [&app_size, &msg_responser](TCPConnection& this_con) {
                        auto check = this_con.try_to_read(app_size);
                        if (check >= app_size) {
                            auto& rbuffer = this_con.get_rb_ref();
                            auto& wbuffer = this_con.get_wb_ref();
                            msg_responser.do_it(rbuffer, wbuffer);
                            this_con.try_to_write();
                        } else {
                            NOTDONE();
                        }
                    })
                .set_peer_close_readable_callback(
                    [](TCPConnection& this_con) { LOG_DEBUG("peer down"); })
                .epoll_and_conmunicate();
        })
        .epoll_and_accept();

    my_connector.set_local_addr(clientip)
        .set_connect_to_addr(listen_ip)
        .set_connect_callback([&clientholder](
            shared_ptr<TCPConnection> shared_p_tc) {
            clientholder.shared_p_tc_vec_.push_back(shared_p_tc);
            LOG_INFO("connected !");
            shared_p_tc->write_by_string("fucking awesome!");
            shared_p_tc
                ->set_normal_readable_callback([](TCPConnection& this_con) {
                })
                .set_peer_close_readable_callback(
                    [](TCPConnection& this_con) { LOG_DEBUG("peer down"); })
                .epoll_and_conmunicate();
        })
        .epoll_and_connect();
    //emw.loop_once(5 * 1000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
