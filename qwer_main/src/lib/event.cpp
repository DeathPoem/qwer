#include "event.h"

namespace my_http {

    EventManager::EventManager() : io_demultiplexer_(new epollwrapper()), exit_(false){
        io_demultiplexer_->Init();
    }

    EventManager::~EventManager() { }

    void EventManager::register_event(EventEnum para_enum, Channel* p_ch, CallBack&& cb) {
        event_call_back_map_[make_pair(para_enum, p_ch)] = cb;
    }
    
    void EventManager::handle_event(EventEnum para_enum, Channel* p_ch) {
        auto callback = event_call_back_map_[make_pair(para_enum, p_ch)];
        callback();
    }

    void EventManager::loop_once(time_ms_t timeout) {
        assert(io_demultiplexer_ != nullptr);
        io_demultiplexer_->loop_once(timeout);
    }

    void EventManager::exit() {
        exit_ = true;
    }

    void EventManager::loop() {
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
        active_channels_.push_back(make_pair(para_enum, p_ch));
    }

    void EventManager::handle_active_event() {
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
    }

    __thread EventManagerWrapper* this_thread_emw_p_thread_local = nullptr;

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

    EventManagerWrapper::~EventManagerWrapper() {}

    void EventManagerWrapper::loop_once(time_ms_t timeout) {
        assert(pimpl_ != nullptr);
        pimpl_->loop_once(timeout);
    }

    void EventManagerWrapper::register_event(uint32_t event, Channel* p_ch, CallBack&& cb) {
        EventEnum para_enum;
        if (event == EPOLLIN) {
            para_enum = EventEnum::IORead;
        } else if (event == EPOLLOUT) {
            para_enum = EventEnum::IOWrite;
        } else {
            NOTDONE();
        }
        pimpl_->register_event(para_enum, p_ch, std::move(cb));
    }

    void EventManagerWrapper::loop() {
        pimpl_->loop();
    }

    void EventManagerWrapper::exit() {
        pimpl_->exit();
    }
} /* my_http */ 
