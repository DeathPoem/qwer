#ifndef EPOLLWRAPPER_H
#define EPOLLWRAPPER_H value

#include "facility.h"
#include "event.h"
#include "my_timer.h"
#include <sys/epoll.h>

namespace my_http {

    class EventManager;

    // name it as 'channel', which wraps the operation of fd that can be epoll or poll
    class Channel : private noncopyable {
        public:
            Channel (int fd);
            virtual ~Channel ();
            int get_fd();
            void close();
            bool is_closed();
            //struct epoll_event* get_epoll_event_p(); this is not good, because if you return a heap pointer, the caller would be responsible to delete it.
            // following would be more good
            uint32_t get_events();
            static uint32_t get_readonly_event_flag();
            static uint32_t get_writeonly_event_flag();
            static uint32_t get_wr_event_flag();
            static uint32_t get_no_wr_event_flag();
            void set_events(uint32_t para_event);
        private:
            int fd_;
            uint32_t event_;
            bool is_closed_;
            /* data */
    };

    class IODemultiplexerInterface : private noncopyable {
        public:
            IODemultiplexerInterface ();
            virtual ~IODemultiplexerInterface ();

            // one loop of the event loop
            virtual void loop_once(time_ms_t time = 99) = 0;
            // add Fdwrapper
            virtual void AddChannel(Channel* p_ch) = 0;
            // remove Fdwrapper
            virtual void DelChannel(Channel* p_ch) = 0;
            // update Fdwrapper
            virtual void ModChannel(Channel* p_ch) = 0;
            // init
            virtual void Init(int size = 9999) = 0;
        protected :
            weak_ptr<EventManager> event_manager_;
        private:
            /* data */
    };   

    // TODO add error handling for this class
    class epollwrapper : public IODemultiplexerInterface {
        public:
            epollwrapper ();
            virtual ~epollwrapper ();
            void loop_once(time_ms_t time) override;
            void AddChannel(Channel* p_ch) override;
            void DelChannel(Channel* p_ch) override;
            void ModChannel(Channel* p_ch) override;
            void Init(int size = 9999) override;
        private:
            /* data */
            int epoll_instance_fd_;
            const int MaxEvents = 1000;
            vector<struct epoll_event> epoll_vec_;
            unordered_set<Channel*> active_channels_;
            weak_ptr<EventManager> event_manager_;
    };

    class pollwrapper : public IODemultiplexerInterface {
        public:
            pollwrapper ();
            virtual ~pollwrapper ();

        private:
    };

} /* my_http */ 

#endif /* ifndef EPOLLWRAPPER_H */
