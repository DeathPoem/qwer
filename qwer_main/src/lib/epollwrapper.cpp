#include "epollwrapper.h"
#include "facility.h"

namespace my_http { 

    
    IODemultiplexerInterface::IODemultiplexerInterface() {}

    IODemultiplexerInterface::~IODemultiplexerInterface() {}

    epollwrapper::epollwrapper(EventManager* emp) : emp_(emp), epoll_vec_(vector<struct epoll_event>(MaxEvents)) {}

    epollwrapper::~epollwrapper() {}

    void epollwrapper::Init(int size) {
        epoll_instance_fd_ = ::epoll_create(size);
        if (epoll_instance_fd_ < 0) {
            NOTDONE();
        }
    }

    void epollwrapper::AddChannel(Channel* p_ch) {
        struct epoll_event epoll_event_ob;
        memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
        epoll_event_ob.events = p_ch->get_events();
        epoll_event_ob.data.ptr = p_ch;
        assert(epoll_instance_fd_ >= 0);
        int check = ::epoll_ctl(epoll_instance_fd_, EPOLL_CTL_ADD, p_ch->get_fd(), &epoll_event_ob);
        if (check < 0) {
            if (errno == EEXIST) {
                ModChannel(p_ch);
            } else {
                SLOG_ERROR("errno = " << strerror(errno));
                NOTDONE();
            }
        }
    }

    void epollwrapper::ModChannel(Channel* p_ch) {
        struct epoll_event epoll_event_ob;
        memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
        epoll_event_ob.events = p_ch->get_events();
        epoll_event_ob.data.ptr = p_ch;
        int check = ::epoll_ctl(epoll_instance_fd_, EPOLL_CTL_MOD, p_ch->get_fd(), &epoll_event_ob);
        if (check < 0) {
            NOTDONE();
        }
    }

    // Channel object is owned by TCPconnection ob
    void epollwrapper::DelChannel(Channel* p_ch) {
        auto check = ::epoll_ctl(epoll_instance_fd_, EPOLL_CTL_DEL, p_ch->get_fd(), NULL);
        if (check != 0) {
            LOG_ERROR("fail to delchannel");
        }
    }

    void epollwrapper::loop_once(time_ms_t time) {
        LOG_INFO("enter epollwrapper::loop_once loop timeout = %d milisec, now is %s", time, TimeStamp().init_stamp_of_now().tostring().c_str());
        bool complete_once = false;
        while (!complete_once) {
            int ready = epoll_wait(epoll_instance_fd_, &*epoll_vec_.begin(), MaxEvents, time);
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ABORT("errno=%s", strerror(errno));
            } else if (ready == 0) {
                LOG_INFO("this epoll_wait return zero");
            }
            while (ready-- > 0) {
                Channel* p_ch = static_cast<Channel*>(epoll_vec_[ready].data.ptr);
                int active_event = epoll_vec_[ready].events;
                if (p_ch) {
                    if (active_event == Channel::get_readonly_event_flag()) {
                        p_ch->act(EventEnum::IORead);
                    } else if (active_event == Channel::get_writeonly_event_flag()) {
                        p_ch->act(EventEnum::IOWrite);
                    } else if (active_event & Channel::get_peer_shutdown_flag()) {
                        p_ch->act(EventEnum::PeerShutDown);
                    } else if (active_event & Channel::get_err_flag()) {
                        p_ch->act(EventEnum::Error);
                    } else {
                        NOTDONE();
                    }
                } else {
                    NOTDONE();
                }
            }
            complete_once = true;
        }
    }

} /* my_http */ 
