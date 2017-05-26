#include "epollwrapper.h"
#include "facility.h"

namespace my_http { 

    Channel::Channel(int fd) : fd_(fd) {}

    Channel::~Channel() {}

    int Channel::get_fd() {return fd_;}

    bool Channel::is_closed() {return is_closed_;}

    void Channel::close() {::close(fd_);}

    uint32_t Channel::get_events() {return event_;}

    void Channel::set_events(uint32_t para_event) {event_ = para_event;}

    uint32_t Channel::get_readonly_event_flag() {return EPOLLIN;}

    uint32_t Channel::get_writeonly_event_flag() {return EPOLLOUT;}

    uint32_t Channel::get_no_wr_event_flag() {return EPOLLERR;}

    uint32_t Channel::get_wr_event_flag() {return EPOLLIN | EPOLLOUT;}

    IODemultiplexerInterface::IODemultiplexerInterface() {}

    IODemultiplexerInterface::~IODemultiplexerInterface() {}

    epollwrapper::epollwrapper() : epoll_vec_(vector<struct epoll_event>(MaxEvents)) {}

    epollwrapper::~epollwrapper() {}

    void epollwrapper::Init(int size) {
        epoll_instance_fd_ = epoll_create(size);
        if (epoll_instance_fd_ < 0) {
            // TODO postcondition check
                    NOTDONE();
        }
    }

    void epollwrapper::AddChannel(Channel* p_ch) {
        struct epoll_event epoll_event_ob;
        memset(&epoll_event_ob, 0, sizeof(epoll_event_ob));
        epoll_event_ob.events = p_ch->get_events();
        epoll_event_ob.data.ptr = p_ch;
        int check = epoll_ctl(epoll_instance_fd_, EPOLL_CTL_ADD, p_ch->get_fd(), &epoll_event_ob);
        if (check < 0) {
            // TODO handle error
                    NOTDONE();
        }
    }

    void epollwrapper::ModChannel(Channel* p_ch) {
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
        if (p_ch->is_closed()) {
            active_channels_.erase(active_channels_.find(p_ch));
            epoll_ctl(epoll_instance_fd_, EPOLL_CTL_DEL, p_ch->get_fd(), NULL);
        }
    }

    void epollwrapper::loop_once(time_ms_t time) {
        int ready = epoll_wait(epoll_instance_fd_, &*epoll_vec_.begin(), MaxEvents, time);
        if (ready < 0) {
            // TODO handle error
                    NOTDONE();
        }
        while (ready-- > 0) {
            Channel* p_ch = static_cast<Channel*>(epoll_vec_[ready].data.ptr);
            int active_event = epoll_vec_[ready].events;
            if (p_ch) {
                if (auto shar_em_ = event_manager_.lock()) {
                    if (active_event & EPOLLIN) {
                        shar_em_->add_active_event(EventEnum::IORead, p_ch);
                    } else if (active_event & EPOLLOUT) {
                        shar_em_->add_active_event(EventEnum::IOWrite, p_ch);
                    } else {
                        //TODO
                        NOTDONE();
                    }
                } else {
                    // TODO 
                    NOTDONE();
                }
            } else {
                // TODO 
                    NOTDONE();
            }
        }
        if (auto shar_em_ = event_manager_.lock()) {
            shar_em_->handle_active_event();
        }
    }

} /* my_http */ 
