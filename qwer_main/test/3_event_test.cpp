#include "event.h"
#include "facility.h"
#include "gtest/gtest.h"
#include <sys/timerfd.h>
#include <thread>

using namespace my_http;

TEST(test_case_3, test_event_loop_do_nothing) {
    EventManagerWrapper emw;
    emw.loop_once(5 * 1000);
}

void threadFunc() {
    EventManagerWrapper emw;
    emw.loop_once(5 * 1000);
}

TEST(test_case_3, test_event_loop_thread) {
    EventManagerWrapper emw;
    std::thread thread_0(threadFunc);
    thread_0.join();
    emw.loop_once(1 * 1000);
}

TEST(test_case_3, test_event_loop_timerfd) {
    string outer;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC_COARSE, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel ch_1(timerfd);
    EventManagerWrapper emw;
    auto cb = [&outer, &emw](){ 
        outer = "fucking awesome!";
        emw.exit();
    };
    emw.register_event(Channel::get_readonly_event_flag(), &ch_1, std::move(cb));

    struct itimerspec howlong;
    memset(&howlong, 0, sizeof(howlong));
    howlong.it_interval.tv_sec = 3;
    ::timerfd_settime(timerfd, 0, &howlong, nullptr);
    
    emw.loop();
    ::close(timerfd);
    EXPECT_EQ(outer, "fucking awesome!");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
