#ifndef MY_TIMER_H
#define MY_TIMER_H value

#include "facility.h"
#include <sys/timerfd.h>
#include <queue>

namespace my_http {
    class EventManager;
    class Channel;

    class Timer : private noncopyable {
        public:
            Timer (TimeStamp&& when, CallBack&& cb, unique_id_t seqno);
            virtual ~Timer ();
            Timer(Timer&& other);
            Timer& operator=(Timer&& other);
            TimerId get_timerid();
            void do_cb();

        private:
            TimerId timerid_;
            CallBack cb_;
    };

    namespace detail {

        TimeStamp create_big_enough_timestamp();
        int creat_timerfd_mine();

    } /* detail */ 

    // one timerfd for all timer callback
    class TimerQueue : private noncopyable{
        public:
            TimerQueue (EventManager& ref_em);
            virtual ~TimerQueue ();
            TimerId add_timer(TimeStamp when, CallBack&& cb);
            void cancel_timer(TimerId);

        private:
            void handle_timeout();
            void init_timerfd_iodemultiplex();
            void after_add_new_timer(TimerId id);
            int remove_dead_timestamp();
            void modify_timerfd_next_time();
            //int timerfd_;
            unique_ptr<Channel> up_ch_;
            // it's overkill to use smart pointer here
            EventManager& ref_em_;
            TimeStamp next_wake_time_;
            std::priority_queue<TimerId, vector<TimerId>, std::greater<TimerId>> timerid_not_active;
            unique_id_t cur_seqence_number_;
            vector<Timer> timer_vec_;
    };

} /* my_timer */ 
#endif /* ifndef MY_TIMER_H */
