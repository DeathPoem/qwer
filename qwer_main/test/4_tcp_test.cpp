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
#include "msg_codec.h"
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
    ::memset(buf, '\0', 99);
    ::memset(buff, '\0', 99);
    {
        std::unique_lock<std::mutex> lk(m);
        LOG_DEBUG("wait for state == 1");
        cv.wait(lk, []() { return state == 1; });
        LOG_DEBUG("block until accept!");
        state = 2;
        using namespace std::chrono_literals;
        while (check != 0) {
            std::this_thread::sleep_for(1s);
            LOG_INFO("connect once");
            check = ::connect(socketfd, (sockaddr *) &peer, peerlen);
            if (check != 0) {
                LOG_INFO("connect , errno =", strerror((errno)));
            }
        }
        LOG_DEBUG("after connect!");
        ::sprintf(buf, "hello? world!");
    }
    cv.notify_one();
    {
        std::unique_lock<std::mutex> lk(m);
        LOG_DEBUG("wait for state == 3");
        cv.wait(lk, []() { return state == 3; });
        LOG_DEBUG("f");
        check = ::write(socketfd, buf, strlen(buf));
        SLOG_DEBUG("after client write! check =" << check << ",buflen="
                                                 << ::strlen(buf));
        assert(check == (int)::strlen(buf));
        if (check < 0) {
            NOTDONE();
        }
        state = 4;
    }
    cv.notify_one();
    {
        std::unique_lock<std::mutex> lk(m);
        LOG_DEBUG("wait for state == 5");
        cv.wait(lk, []() { return state == 5; });
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
        {
            std::lock_guard<std::mutex> lk(m);
            LOG_DEBUG("f");
            state = 1;
        }
        cv.notify_one();
        {
            std::unique_lock<std::mutex> lk(m);
            LOG_DEBUG("wait for state == 2");
            cv.wait(lk, []() { return state == 2;});
            LOG_DEBUG("accept invoked!");
            new_fd = ::accept(socketfd, &peer, &peerlen);
            SLOG_DEBUG("after accept " << i
                                        << " state =" << state);
            state = 3;
        }
        if (new_fd < 0) {
            NOTDONE();
        }
        cv.notify_one();
        {
            std::unique_lock<std::mutex> lk(m);
            LOG_DEBUG("wait for state == 4");
            cv.wait(lk, []() { return state == 4; });
            state = 5;
            echo(new_fd);
            LOG_DEBUG("after echo");
        }
        cv.notify_one();
    }
    client_thread.join();
    ::close(socketfd);
}

struct TCPConnectionHolder {
    vector<shared_ptr<TCPConnection>> shared_p_tc_vec_;
};

struct XMsgResponser {
    void do_it(Buffer& rb, Buffer& wb) {
        char readto[4000];
        memset(readto, 0, sizeof readto);
        assert(rb.get_readable_bytes() < 4000);
        assert(rb.get_readable_bytes() > 0);
        rb.read_from_buffer(readto, rb.get_readable_bytes());
        auto consume_size = strlen(readto);
        rb.consume(consume_size);
        char result[4000];
        memset(result, 0, sizeof result);
        memcpy(result, readto, strlen(readto));
        wb.write_to_buffer(result, strlen(result));
    }
    void do_it(char* msg, char* after) { ::memcpy(msg, after, strlen(msg)); }
    void client_do_it(Buffer& rb, Buffer& wb) {
        char writefrom[4000];
        sprintf(writefrom, "this is client, hello?");
        wb.write_to_buffer(writefrom, strlen(writefrom));
    }
};

void client_thread_function() {
    EventManagerWrapper emw;
    Ipv4Addr listen_ip(Ipv4Addr::host2ip_str("localhost"), 3876);
    Ipv4Addr clientip(Ipv4Addr::host2ip_str("localhost"), 86211);
    TCPConnectionHolder clientholder;
    XMsgResponser msg_responser;
    Connector my_connector(&emw);
    string outer;
    my_connector.set_local_addr(clientip)
            .set_connect_to_addr(listen_ip)
            .set_connect_callback([&outer, &clientholder](
                    shared_ptr<TCPConnection> shared_p_tc) {
                clientholder.shared_p_tc_vec_.push_back(shared_p_tc);
                LOG_DEBUG("connected and write to server!");
                shared_p_tc->write_by_string("fucking awesome!");
                shared_p_tc
                        ->set_normal_readable_callback([&outer](TCPConnection& this_con) {
                            LOG_DEBUG("read from server");
                            outer = this_con.read_by_string();
                            this_con.get_rb_ref().consume(outer.size());
                            this_con.local_close();
                        })
                        .set_local_close_callback(
                                [&clientholder](TCPConnection& this_con) {
                                    LOG_DEBUG("client close");
                                    clientholder.shared_p_tc_vec_.clear();  // shared_ptr would destruct
                                })
                        .epoll_and_conmunicate();
            })
            .epoll_and_connect();
    for (int c1 : {1, 2, 3, 4, 5}) {
        SLOG_INFO("client " << c1 << " times loop_once");
        emw.loop_once(1 * 1000);
    }
    EXPECT_EQ(outer, "fucking awesome!");
}

void server_thread_function() {
    EventManagerWrapper emw;
    Acceptor acceptor(&emw);
    Ipv4Addr listen_ip(Ipv4Addr::host2ip_str("localhost"), 3876);
    TCPConnectionHolder holder;
    XMsgResponser msg_responser;
    acceptor.set_listen_addr(listen_ip)
            .set_accept_readable_callback([&holder, &msg_responser](
                    shared_ptr<TCPConnection> shared_p_tc) {
                LOG_DEBUG("accept success");
                holder.shared_p_tc_vec_.push_back(shared_p_tc);
                shared_p_tc
                        ->set_normal_readable_callback(
                                [&msg_responser](TCPConnection& this_con) {
                                    LOG_DEBUG("read from client");
                                    size_t check = this_con.try_to_read();
                                    if (check >= 1) {
                                        LOG_DEBUG("read from client as %d", check);
                                        auto& rbuffer = this_con.get_rb_ref();
                                        auto& wbuffer = this_con.get_wb_ref();
                                        msg_responser.do_it(rbuffer, wbuffer);
                                        this_con.try_to_write();
                                    } else {
                                        LOG_WARN("read empty, why?");
                                    }
                                    LOG_DEBUG("end read from client");
                                })
                        .set_peer_close_callback(
                                [](TCPConnection& this_con) {
                                    LOG_DEBUG("client down");
                                })
                        .epoll_and_conmunicate();
            })
            .epoll_and_accept();
    for (int c1 : {1, 2, 3, 4, 5, 6, 7, 8, 9}) {
        SLOG_INFO("server " << c1 << " times loop_once");
        emw.loop_once(1 * 1000);
    }
}


TEST(test_case_4, test_tcp_acceptor_connector) {
    LOG_SET_FILE_P("", true);
    LOG_SET_LEVEL("INFO");

    auto server_thread = std::thread(server_thread_function);
    auto client_thread = std::thread(client_thread_function);
    client_thread.join();
    server_thread.join();
}

static int is_server_thread_down_;

void x_server_thread_function() {
    EventManagerWrapper emw;
    EchoMsgResponser msg_responser;
    Ipv4Addr listen_ip(Ipv4Addr::host2ip_str("localhost"), 53176);
    TCPServer tcpserver(&emw, listen_ip);
    tcpserver.set_msg_responser_callback([&msg_responser](uint32_t seqno, Buffer& rb, Buffer& wb, BigFileSendCallBack&& bfcb) {
                LOG_DEBUG("read from client");
                msg_responser.do_it_for_con_of_seqno(seqno, rb, wb);
            });
    for (int i = 0; i < 20; ++i) {
        emw.loop_once(500);
    }
    assert(is_server_thread_down_ == 0);
    is_server_thread_down_ = 1;
}

void x_client_thread_function() {
    EventManagerWrapper emw;
    string write_to_server = "fucking awesome!", read_from_server = "";
    Ipv4Addr server_ip(Ipv4Addr::host2ip_str("localhost"), 53176);
    Ipv4Addr local_ip(Ipv4Addr::host2ip_str("localhost"), 32321);
    TCPClient tcpclient(&emw, server_ip, local_ip);
    tcpclient.set_tcpcon_after_connected_callback([](TCPConnection& this_con){
                LOG_DEBUG("write to server");
                this_con.write_by_string("fucking awesome!");
                }).set_msg_callback([&tcpclient, &write_to_server, &read_from_server](uint32_t seqno) {
                        LOG_DEBUG("read from server, %d", is_server_thread_down_);
                        auto this_con = tcpclient.get_shared_tcpcon_ref();
                        string re = this_con->read_by_string();
                        read_from_server = re == "" ? read_from_server : re;
                        this_con->get_rb_ref().consume(read_from_server.size());
                        this_con->local_close();
                    });
    for (int i = 0; i < 10; ++i) {
        emw.loop_once(500);
    }
    EXPECT_EQ(read_from_server, write_to_server);
}

TEST(test_case_4, test_tcp_server_client) {
    LOG_SET_FILE_P("", true);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_tcp_server_client");

    auto server_thread = std::thread(x_server_thread_function);
    auto client_thread = std::thread(x_client_thread_function);
    client_thread.join();
    server_thread.join();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
