#ifndef MY_TIMER_H
#define MY_TIMER_H value

#include "facility.h"
#include <sys/timerfd.h>

namespace my_http {
    using time_ms_t = int;

    struct TimeStamp {

    };

    struct TimerId {
        TimeStamp alarm_time_;
        unique_id_t seqno_;
    };

    class Timer : private noncopyable {
    public:
        Timer (TimeStamp&& when, CallBack&& cb, unique_id_t seqno);
        virtual ~Timer ();
    
    private:
        TimerId timerid_;
        CallBack cb_;
    };

    namespace detail {

        int creat_timerfd_mine();

    } /* detail */ 

    // one timerfd for all timer callback
    class TimerQueue : private noncopyable{
    public:
        TimerQueue ();
        virtual ~TimerQueue ();
        TimerId add_timer();
        void cancel_timer(TimerId);
    
    private:
        int timerfd_;
        unique_id_t cur_seqence_number_;
        vector<Timer> timer_vec_;
    };
    
} /* my_timer */ 
#endif /* ifndef MY_TIMER_H */
