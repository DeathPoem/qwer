#include "my_timer.h"
#include "facility.h"
#include "event.h"
#include <time.h>
#include <math.h>


namespace my_http {

    Timer::Timer(TimeStamp&& when, CallBack&& cb, unique_id_t seqno, time_ms_t interval) : cb_(cb), interval_(interval) {
        timerid_.alarm_time_ = when;
        timerid_.seqno_ = seqno;
    }

    Timer::~Timer() {}

    bool Timer::is_period() {
        return interval_ > 0;
    }

    void Timer::to_next_time_if_period() {
        if (is_period()) {
            TimeStamp when;
            when.init_stamp_of_now().add_stamp_by_mill(interval_);
            timerid_.alarm_time_ = when;
        } else {
            ABORT("no interval");
        }
    }

    Timer::Timer(Timer&&  other) : timerid_(std::move(other.timerid_)),
    cb_(std::move(other.cb_)) { }

    TimerId Timer::get_timerid() {
        return timerid_;
    }

    Timer& Timer::operator=(Timer&& other) {
        if (this != &other) {
            timerid_ = std::move(other.timerid_);
            cb_ = std::move(other.cb_);
        }
        return *this;
    }

    void Timer::do_cb() { cb_(); }

    TimerQueue::TimerQueue(EventManager& ref_em) : ref_em_(ref_em), up_ch_(new Channel(detail::creat_timerfd_mine())) {
        up_ch_->add_event(Channel::get_readonly_event_flag());
    }

    TimerQueue::~TimerQueue() {
        if (!timer_vec_.empty()) {
            SLOG_WARN("timer vec is not empty! size =" << to_string(timer_vec_.size()));
        }
    }

    int detail::creat_timerfd_mine() {
        LOG_INFO("here");
        int tmp_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        assert(tmp_fd >= 0);
        return tmp_fd;
    }

    TimeStamp detail::create_big_enough_timestamp() {
        LOG_INFO("here");
        TimeStamp ts = TimeStamp().init_stamp_of_now(); 
        ts.add_stamp_by_mill(1000 * 1000);
        return std::move(ts);
    }

    TimerId TimerQueue::add_timer(TimeStamp when, CallBack&& cb, time_ms_t interval) {
        LOG_INFO("here");
        if (timer_vec_.empty()) {
            LOG_INFO("init timer and register it");
            init_timerfd_iodemultiplex();
        }
        timer_vec_.emplace_back(std::move(when), std::move(cb), cur_seqence_number_++, interval);
        auto id = timer_vec_.back().get_timerid();
        after_add_new_timer(id);
        return id;
    }

    void TimerQueue::handle_timeout() {
        LOG_INFO("here, handle_timeout");
        // handle all timer before current time
        auto now = TimeStamp().init_stamp_of_now();
        vector<TimerId> to_erase_timerid;
        for_each(timer_vec_.begin(), timer_vec_.end(), [&now, &to_erase_timerid](Timer& timer){
                LOG_INFO("this timer alert = %s sec, now = %s", timer.get_timerid().alarm_time_.tostring().c_str(), now.tostring().c_str());
                if (timer.get_timerid().alarm_time_ < now) {
                timer.do_cb();
                to_erase_timerid.push_back(timer.get_timerid());
                }
                });
        // remove times_not_active_
        int check = remove_dead_timestamp();
        if (check != to_erase_timerid.size()) {
            NOTDONE();
        }
        // remove timer in timer_vec_
        for (auto id : to_erase_timerid) {
            auto found = find_if(timer_vec_.begin(), timer_vec_.end(), [&id](Timer& timer){ return timer.get_timerid() == id; });
            if (found->is_period()) {
                found->to_next_time_if_period();
                timerid_not_active.push(found->get_timerid());
            } else {
                timer_vec_.erase(found);
            }
        }
        modify_timerfd_next_time();
    }

    int TimerQueue::remove_dead_timestamp() {
        LOG_INFO("here, remove_dead_timestamp");
        auto now = TimeStamp().init_stamp_of_now();
        LOG_INFO("top = %s, now = %s", timerid_not_active.top().alarm_time_.tostring().c_str(), now.tostring().c_str());
        int count = 0;
        while (timerid_not_active.top().alarm_time_ < now && !timerid_not_active.empty()) {
            timerid_not_active.pop();
            count++;
        }
        return count;
    }

    void TimerQueue::after_add_new_timer(TimerId id) {
        LOG_INFO("here, after_add_new_timer");
        timerid_not_active.push(id);
        if (timerid_not_active.top().alarm_time_ < next_wake_time_) {
            modify_timerfd_next_time();
        }
    }

    void TimerQueue::modify_timerfd_next_time() {
        LOG_INFO("here, modify_timerfd_next_time");
        if (timerid_not_active.empty()) {
            LOG_INFO("no timer not active, delete register and return");
            assert(timer_vec_.empty());
            ref_em_.remove_registered_event(Channel::get_readonly_event_flag(), up_ch_.get());
            return;
        }
        next_wake_time_ = timerid_not_active.top().alarm_time_;
        auto timer_fd = up_ch_->get_fd();
        struct itimerspec howlong;
        memset(&howlong, 0, sizeof(howlong));
        howlong.it_value = timerid_not_active.top().alarm_time_.get_spec();
        int check;
        check = ::timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &howlong, nullptr);
        if (check != 0) {
            struct itimerspec old_time;
            int check1 = ::timerfd_gettime(timer_fd, &old_time);
            if (check1 != 0) {
                NOTDONE();
            }
            TimeStamp ts1(old_time.it_value), ts2(old_time.it_interval);
            SLOG_DEBUG("our old time is " << ts1 << ",interval is" << ts2 << ",str errorno is" << std::strerror(errno));
            NOTDONE();
        }
    }

    void TimerQueue::init_timerfd_iodemultiplex() {
        LOG_INFO("here");
        auto event = Channel::get_readonly_event_flag();
        ref_em_.register_event(Channel::get_readonly_event_flag(), up_ch_.get(),
                bind(&TimerQueue::handle_timeout, this));
        next_wake_time_ = detail::create_big_enough_timestamp();
    }
} /* my_http  */ 
