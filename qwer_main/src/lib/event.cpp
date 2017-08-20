#include "event.h"

namespace my_http {
    EventManager::EventManager() : io_demultiplexer_(new epollwrapper(this)),
    exit_(false),
    my_timer_queue_(new TimerQueue(*this)) {
        io_demultiplexer_->Init();
    }

    EventManager::~EventManager() { }

    /*
    void EventManager::register_event(Channel* p_ch, CallBack&& cb) {
        LOG_INFO("enter EventManager::register_event");
        event_call_back_map_[make_pair(uint2enum(p_ch->get_events()), p_ch)] = std::move(cb);
        io_demultiplexer_->AddChannel(p_ch);
    }
     */

    void EventManager::register_event(uint32_t event, Channel* p_ch, CallBack&& cb) {
        LOG_INFO("enter EventManager::register_event");
        p_ch->add_event(event);
        p_ch->add_cb(event, move(cb));
        io_demultiplexer_->AddChannel(p_ch);
    }

    void EventManager::remove_registered_event(uint32_t event, Channel *p_ch) {
        LOG_INFO("enter EventManager::remove_registered_event");
        p_ch->delete_event(event);
        io_demultiplexer_->ModChannel(p_ch);
    }

    void EventManager::remove_channel(Channel *p_ch) {
        LOG_INFO("enter EventManager::remove_registered_event");
        io_demultiplexer_->DelChannel(p_ch);
        p_ch->delete_event(p_ch->get_events());
    }
    
    void EventManager::loop_once(time_ms_t timeout) {
        LOG_INFO("loop once");
        assert(io_demultiplexer_ != nullptr);
        io_demultiplexer_->loop_once(timeout);
    }

    TimerId EventManager::run_at(time_ms_t para_t, CallBack&& cb, time_ms_t interval) {
        LOG_INFO("run at");
        return run_after(para_t, std::move(cb), interval);
    }

    TimerId EventManager::run_after(time_ms_t para_t, CallBack&& cb, time_ms_t interval) {
        if (interval == 0) {
            TimeStamp when;
            when.init_stamp_of_now().add_stamp_by_mill(para_t);
            auto ret = my_timer_queue_->add_timer(when, move(cb));
            return ret;
        } else {
            TimeStamp when;
            when.init_stamp_of_now().add_stamp_by_mill(para_t);
            auto ret = my_timer_queue_->add_timer(when, move(cb), interval);
            return ret;
        }
    }

    void EventManager::exit() {
        LOG_DEBUG("event loop exit");
        exit_ = true;
    }

    void EventManager::loop() {
        while (!exit_) {
            io_demultiplexer_->loop_once(300);
        }
    }
    
    void EventManager::start_up() {}

    /*
    void EventManager::add_active_event(EventEnum para_enum, Channel* p_ch) {
        SLOG_INFO("enter EventManager::add_active_event" << ";event=" << para_enum);
    }
    */

    /*
    void EventManager::handle_active_event() {
        LOG_INFO("enter EventManager::handle_active_event : active_channels_.size() = %d; event_call_back_map_.size() = %d", 
                active_channels_.size(), 
                event_call_back_map_.size());
        for (auto& va : active_channels_) {
            auto found = event_call_back_map_.find(va);
            if (found != event_call_back_map_.end()) {
                get<1>(*found)();
            } else {
                NOTDONE();
            }
        }
        active_channels_.clear();
        assert(active_channels_.size() == 0);
    }
    */

    thread_local EventManagerWrapper* this_thread_emw_p_thread_local = nullptr;

    EventManagerWrapper::EventManagerWrapper() {
        // it's safe to call start_up for pimpl_, because, though EventManagerWrapper is not full constructed, it's member, pimpl_ is.
        if (this_thread_emw_p_thread_local) {
            ABORT("one EventManagerWrapper exist in this thread!");
        } else {
            this_thread_emw_p_thread_local = this;
        }
        pimpl_.reset(new EventManager());
        pimpl_->start_up();
    }

    TimerId EventManagerWrapper::run_after(time_ms_t para_t, CallBack &&cb, time_ms_t interval) {
        auto ret = pimpl_->run_at(para_t, std::move(cb), interval);
        return ret;
    }

    EventManagerWrapper::~EventManagerWrapper() {
        this_thread_emw_p_thread_local = nullptr;
    }

    void EventManagerWrapper::loop_once(time_ms_t timeout) {
        assert(pimpl_ != nullptr);
        pimpl_->loop_once(timeout);
    }

    void EventManagerWrapper::register_event(uint32_t event, Channel* p_ch, CallBack&& cb) {
        pimpl_->register_event(event, p_ch, std::move(cb));
    }

    void EventManagerWrapper::loop() {
        pimpl_->loop();
    }

    EventManager* EventManagerWrapper::get_pimpl() {
        return pimpl_.get();
    }

    void EventManagerWrapper::exit() {
        pimpl_->exit();
    }
} /* my_http */ 
