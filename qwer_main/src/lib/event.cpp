#include "event.h"

namespace my_http {

    EventManager::EventManager() : io_demultiplexer_(new epollwrapper(this)),
    exit_(false),
    my_timer_queue_(new TimerQueue(std::ref(*this))) {
        io_demultiplexer_->Init();
    }

    EventManager::~EventManager() { }

    void EventManager::register_event(Channel* p_ch, CallBack&& cb) {
        LOG_INFO("enter EventManager::register_event");
        event_call_back_map_[make_pair(uint2enum(p_ch->get_events()), p_ch)] = std::move(cb);
        io_demultiplexer_->AddChannel(p_ch);
    }
    
    void EventManager::handle_event(EventEnum para_enum, Channel* p_ch) {
        LOG_INFO("enter EventManager::handle_event");
        LOG_DEBUG("map size= %d", event_call_back_map_.size());
        auto callback = event_call_back_map_[make_pair(para_enum, p_ch)];
        callback();
    }

    void EventManager::loop_once(time_ms_t timeout) {
        LOG_INFO("here");
        assert(io_demultiplexer_ != nullptr);
        io_demultiplexer_->loop_once(timeout);
    }

    TimerId EventManager::run_at(time_ms_t para_t, CallBack&& cb) {
        LOG_INFO("here");
        TimeStamp when;
        when.init_stamp_of_now().add_stamp_by_mill(para_t);
        auto ret = my_timer_queue_->add_timer(when, move(cb));
        LOG_INFO("here");
        return ret;
    }

    void check_state_of_timerid(TimerId tid) {}

    void EventManager::exit() {
        exit_ = true;
    }

    void EventManager::loop() {
        LOG_INFO("here");
        int anti_dead_loop = 0;
        while (!exit_ && anti_dead_loop++ < 40) {
            io_demultiplexer_->loop_once(1000);
        }
        if (!exit_) {
            //TODO handle some
        }
    }
    
    void EventManager::start_up() {}

    void EventManager::add_active_event(EventEnum para_enum, Channel* p_ch) {
        LOG_INFO("enter EventManager::add_active_event");
        active_channels_.push_back(make_pair(para_enum, p_ch));
    }

    void EventManager::handle_active_event() {
        LOG_INFO("enter EventManager::handle_active_event : active_channels_.size() = %d; event_call_back_map_.size() = %d", 
                active_channels_.size(), 
                event_call_back_map_.size());
        for (auto va : active_channels_) {
            auto found = event_call_back_map_.find(va);
            if (found != event_call_back_map_.end()) {
                std::get<1>(*found)();
            } else {
                // TODO : not found register_event
                NOTDONE();
            }
        }
        active_channels_.clear();
        assert(active_channels_.size() == 0);
    }

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

    TimerId EventManagerWrapper::run_at(time_ms_t para_t, CallBack&& cb) {
        auto ret = pimpl_->run_at(para_t, std::move(cb));
        LOG_INFO("here");
        return ret;
    }

    EventManagerWrapper::~EventManagerWrapper() {
        this_thread_emw_p_thread_local = nullptr;
    }

    void EventManagerWrapper::loop_once(time_ms_t timeout) {
        assert(pimpl_ != nullptr);
        pimpl_->loop_once(timeout);
    }

    void EventManagerWrapper::register_event(Channel* p_ch, CallBack&& cb) {
        pimpl_->register_event(p_ch, std::move(cb));
    }

    EventEnum uint2enum(uint32_t event) {
        EventEnum para_enum;
        if (event == EPOLLIN) {
            para_enum = EventEnum::IORead;
        } else if (event == EPOLLOUT) {
            para_enum = EventEnum::IOWrite;
        } else {
            NOTDONE();
        }
        return para_enum;
    }

    void EventManagerWrapper::loop() {
        pimpl_->loop();
    }

    void EventManagerWrapper::exit() {
        pimpl_->exit();
    }
} /* my_http */ 