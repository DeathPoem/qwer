#ifndef EPOLLWRAPPER_H
#define EPOLLWRAPPER_H value

#include "facility.h"
#include "event.h"
#include "my_timer.h"
#include <sys/epoll.h>

namespace my_http {

    class EventManager;

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
        private:
            /* data */
    };   

    class epollwrapper : public IODemultiplexerInterface {
        public:
            epollwrapper (EventManager* emp);
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
            EventManager* emp_;
    };

    class pollwrapper : public IODemultiplexerInterface {
        public:
            pollwrapper ();
            virtual ~pollwrapper ();

        private:
    };

} /* my_http */ 

#endif /* ifndef EPOLLWRAPPER_H */
