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

TEST(test_case_5, test_http_in_threadpool) {
    MultiServer ms;
    ms.set_msg_callback([](size_t seqno){

            });
    ThreadPool thread_pool;
    thread_pool.add_task(std::bind(&MultiServer::AcceptorRoutine, &ms));
    thread_pool.add_task(std::bind(&MultiServer::MsgServerRoutine, &ms));
    thread_pool.add_task(std::bind(&MultiServer::MsgServerRoutine, &ms));
    thread_pool.start();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
