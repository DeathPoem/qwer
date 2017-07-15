#include "event.h"
#include "facility.h"
#include "gtest/gtest.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <thread>

using namespace my_http;

TEST(test_case_3, test_event_loop_do_nothing) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    EventManagerWrapper emw;
    emw.loop_once(1 * 1000);
}

void threadFunc() {
    LOG_INFO("in other thread");
    EventManagerWrapper emw;
    emw.loop_once(1 * 1000);
}

TEST(test_case_3, test_event_loop_thread) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    LOG_INFO("in main thread");
    EventManagerWrapper emw;
    std::thread thread_0(threadFunc);
    thread_0.join();
    emw.loop_once(1 * 1000);
}

TEST(test_case_3, test_raw_timerfd) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    string outer;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd <= 0) {
        outer += "??";
    }
    auto cb = [&timerfd, &outer](){
        outer += "cb!";
        struct itimerspec howlong;
        struct timespec now;
        memset(&howlong, 0, sizeof(howlong));
        ::clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
        howlong.it_value.tv_sec = now.tv_sec + 1;
        // first expiration is 3 second later
        int check = timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &howlong, nullptr);
        if (check != 0) {
            outer += "can't modify timerfd?";
        } else {
            outer += "modify timerfd!";
        }
    };
    auto cb1 = [&outer](){outer += "cb!";};

    // epoll 
    int epoll_fd = ::epoll_create(99);
    fcntl(epoll_fd, F_SETFD, O_NONBLOCK);
    struct epoll_event epoll_event_ob, events[100];
    memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
    epoll_event_ob.events = EPOLLIN | EPOLLET;
    epoll_event_ob.data.ptr = nullptr;

    // timerfd
    struct itimerspec howlong;
    struct timespec now;
    memset(&howlong, 0, sizeof(howlong));
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
    howlong.it_value.tv_sec = now.tv_sec + 1;
    // first expiration is 3 second later
    int check = timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &howlong, nullptr);
    if (check != 0) {
        outer += "what?";
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timerfd, &epoll_event_ob);

    for (int i : {1, 2, 3}) {
        int ready = epoll_wait(epoll_fd, events, 5, 1000);
        if (i == 1 && ready == 1) {
            cb();
        } else if (i == 2 && ready == 1) {
            cb1();
        } else {
            outer += "no noticed!";
        }
    }

    ::close(timerfd);
    EXPECT_EQ(outer, "cb!modify timerfd!cb!no noticed!");
}

TEST(test_case_3, test_event_loop_timerfd) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    LOG_DEBUG("what the fuck!");
    string outer;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel ch_1(timerfd);
    EventManagerWrapper emw;
    auto cb = [&outer, &emw](){ 
        outer += "fucking awesome!";
        emw.exit();
    };
    emw.register_event(Channel::get_readonly_event_flag(), &ch_1, std::move(cb));

    struct itimerspec howlong;
    memset(&howlong, 0, sizeof(howlong));
    howlong.it_value.tv_sec = TimeStamp().init_stamp_of_now().get_spec().tv_sec + 1;    // first expiration is second later
    howlong.it_interval.tv_sec = 1;     // every 3 seconds
    ::timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &howlong, nullptr);

    emw.loop_once(2000);
    ::close(timerfd);
    EXPECT_EQ(outer, "fucking awesome!");
}

TEST(test_case_3, test_event_loop_runat) {
    LOG_SET_FILE("");
    LOG_SET_LEVEL("INFO");
    string outer;
    string another;
    string other;
    EventManagerWrapper emw;
    auto cb0 = [&outer](){ outer = "fucking awesome!"; };
    auto cb1 = [&another](){ another = "fucking asshole!"; };
    auto cb2 = [&other](){ other = "fucking!"; };
    emw.run_after(2300, std::move(cb0));
    emw.run_after(900, std::move(cb1));
    emw.run_after(900, std::move(cb2));

    for (int i : {1, 2, 3}) {
        SLOG_INFO("end of i =" << i);
        try {
            emw.loop_once(1 * 1000);
        } catch (const std::exception& e) {
            cout << e.what() << endl;
        }
    }

    EXPECT_EQ(outer, "fucking awesome!");
    EXPECT_EQ(another, "fucking asshole!");
    EXPECT_EQ(other, "fucking!");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
