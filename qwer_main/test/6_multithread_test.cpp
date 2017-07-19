#include "facility.h"
#include "gtest/gtest.h"
#include "multithread_version.h"
#include "tcpserver.h"
#include "threadpool.h"

using namespace my_http;

//void acceptor_routine(
//    AcceptedQueue<FileDescriptorType>*
//        p_queue) {  //!< p_queue lived longer than this routine
//
//    EventManagerWrapper emw;
//    MultiAcceptor x_acceptor(&emw);
//    x_acceptor
//        .set_listen_addr(Ipv4Addr(Ipv4Addr::host2ip_str("local_host"), 8001))
//        .set_accept_readable_callback(
//            [&p_queue](FileDescriptorType fd) { p_queue->push_one(move(fd)); })
//        .epoll_and_accept();
//}
//
//void responser_routine(AcceptedQueue<FileDescriptorType>* p_queue) {
//    EventManagerWrapper emw;
//
//    map<uint32_t, shared_ptr<TCPConnection>> tcpcon_map_;
//}
//
//void client_routine() {}
//
void x_client_thread_function() {
    EventManagerWrapper emw;
    string write_to_server = "fucking awesome!", read_from_server = "";
    Ipv4Addr server_ip(Ipv4Addr::host2ip_str("localhost"), 8001);
    Ipv4Addr local_ip(Ipv4Addr::host2ip_str("localhost"), 32321);
    TCPClient tcpclient(&emw, server_ip, local_ip);
    tcpclient.set_tcpcon_after_connected_callback([](TCPConnection& this_con){
                LOG_DEBUG("write to server");
                this_con.write_by_string("fucking awesome!");
                })
            .set_tcp_callback([&tcpclient, &write_to_server, &read_from_server](TCPConnection& this_con) {
                        LOG_DEBUG("read from server, ");
                        if (this_con.get_state() == TCPSTATE::Localclosed) {
                            NOTDONE();
                        }
                        string re = this_con.read_by_string();
                        read_from_server = re == "" ? read_from_server : re;
                        this_con.get_rb_ref().consume(read_from_server.size());
                        SLOG_DEBUG("client read from server as" << read_from_server);
                        this_con.local_close();
                    });
    for (int i = 0; i < 20; ++i) {
        emw.loop_once(500);
        LOG_DEBUG("one pass->>>>>>>>>>>>\n");
    }
    EXPECT_EQ(read_from_server, write_to_server);
    LOG_DEBUG("client thread end");
}

TEST(test_case_5, test_http_in_threadpool) {
    LOG_SET_FILE_P("", false);
    LOG_SET_LEVEL("DEBUG");
    LOG_DEBUG(" \n \n in test_multithread");
    MultiServer ms;
    EchoMsgResponser msg_responser;
    ms.set_msg_responser_callback([&msg_responser](size_t seqno, Buffer& rb, Buffer& wb, BigFileSendCallBack&& cb) {
                LOG_DEBUG("read from client");
                msg_responser.do_it_for_con_of_seqno(seqno, rb, wb);
            });
    ms.ThreadPoolStart();
    std::this_thread::sleep_for(1000ms);
    auto client_thread = std::thread(x_client_thread_function);
    client_thread.join();
    ms.Exit();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
