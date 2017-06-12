#include "epollwrapper.h"
#include "facility.h"

namespace my_http { 

    
    IODemultiplexerInterface::IODemultiplexerInterface() {}

    IODemultiplexerInterface::~IODemultiplexerInterface() {}

    epollwrapper::epollwrapper(EventManager* emp) : emp_(emp), epoll_vec_(vector<struct epoll_event>(MaxEvents)) {}

    epollwrapper::~epollwrapper() {}

    void epollwrapper::Init(int size) {
        LOG_INFO("here");
        epoll_instance_fd_ = epoll_create(size);
        if (epoll_instance_fd_ < 0) {
            // TODO postcondition check
            NOTDONE();
        }
    }

    void epollwrapper::AddChannel(Channel* p_ch) {
        LOG_INFO("here");
        struct epoll_event epoll_event_ob;
        memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
        epoll_event_ob.events = p_ch->get_events();
        epoll_event_ob.data.ptr = p_ch;
        assert(epoll_instance_fd_ >= 0);
        int check = epoll_ctl(epoll_instance_fd_, EPOLL_CTL_ADD, p_ch->get_fd(), &epoll_event_ob);
        if (check < 0) {
            // TODO handle error
            NOTDONE();
        }
    }

    void epollwrapper::ModChannel(Channel* p_ch) {
        LOG_INFO("here");
        struct epoll_event epoll_event_ob;
        memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
        epoll_event_ob.events = p_ch->get_events();
        epoll_event_ob.data.ptr = p_ch;
        int check = epoll_ctl(epoll_instance_fd_, EPOLL_CTL_MOD, p_ch->get_fd(), &epoll_event_ob);
        if (check < 0) {
            // TODO
                    NOTDONE();
        }
    }

    // Channel object is owned by TCPconnection ob
    void epollwrapper::DelChannel(Channel* p_ch) {
        LOG_INFO("here");
        if (p_ch->is_closed()) {
            active_channels_.erase(active_channels_.find(p_ch));
            epoll_ctl(epoll_instance_fd_, EPOLL_CTL_DEL, p_ch->get_fd(), NULL);
        }
    }

    void epollwrapper::loop_once(time_ms_t time) {
        LOG_INFO("enter epollwrapper::loop_once loop timeout = %d milisec, now is %s", time, TimeStamp().init_stamp_of_now().tostring().c_str());
        int ready = epoll_wait(epoll_instance_fd_, &*epoll_vec_.begin(), MaxEvents, time);
        if (ready < 0) {
            // TODO handle error
            NOTDONE();
        } else if (ready == 0) {
            LOG_INFO("this epoll_wait return zero");
        }
        while (ready-- > 0) {
            Channel* p_ch = static_cast<Channel*>(epoll_vec_[ready].data.ptr);
            int active_event = epoll_vec_[ready].events;
            if (p_ch) {
                if (emp_ != nullptr) {
                    if (active_event & EPOLLIN) {
                        emp_->add_active_event(EventEnum::IORead, p_ch);
                    } else if (active_event & EPOLLOUT) {
                        emp_->add_active_event(EventEnum::IOWrite, p_ch);
                    } else if (active_event & EPOLLRDHUP) {
                        emp_->add_active_event(EventEnum::PeerShutDown, p_ch);
                    } else {
                        NOTDONE();
                    }
                } else {
                    NOTDONE();
                }
            } else {
                NOTDONE();
            }
        }
        emp_->handle_active_event();
    }

} /* my_http */ 
