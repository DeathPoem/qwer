#include "my_timer.h"

namespace my_http {
    
    Timer::Timer(TimeStamp&& when, CallBack&& cb, unique_id_t seqno) : cb_(cb) {
        timerid_.alarm_time_ = when;
        timerid_.seqno_ = seqno;
    }

    Timer::~Timer() {}

    TimerQueue::TimerQueue() {
        timerfd_ = detail::creat_timerfd_mine();
    }

    int detail::creat_timerfd_mine() {
        int tmp_fd = ::timerfd_create(CLOCK_MONOTONIC_COARSE, TFD_CLOEXEC | TFD_NONBLOCK);
        return tmp_fd;
    }
} /* my_http  */ 
