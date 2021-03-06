#ifndef EVENT_H
#define EVENT_H value

#include "facility.h"
#include "epollwrapper.h"
#include "my_timer.h"
#include <memory>

namespace my_http {

    // pre declare
    class Channel;

    class IODemultiplexerInterface;

    class EventManager : private noncopyable {
        public:
            EventManager();
            virtual ~EventManager();
            //void register_event(Channel* p_ch, CallBack&& cb);
            void register_event(uint32_t event, Channel* p_ch, CallBack&& cb);
            void remove_registered_event(uint32_t event, Channel *p_ch);
            void remove_channel(Channel* p_ch);
            TimerId run_at(time_ms_t para_t, CallBack&& cb, time_ms_t interval = 0);    // means after FIXME
            TimerId run_after(time_ms_t para_t, CallBack&& cb, time_ms_t interval = 0); //!< interval = 0 means that it's not an period task
            void loop();
            void loop_once(time_ms_t timeout);
            //void add_active_event(EventEnum para_enum, Channel* p_ch);
            //void handle_active_event();
            void start_up();
            void exit();
        private:
            unique_ptr<IODemultiplexerInterface> io_demultiplexer_;
            unique_ptr<TimerQueue> my_timer_queue_;
            bool exit_;
            /* data */
    };

    class EventManagerWrapper : private noncopyable {
        public:
            EventManagerWrapper ();
            virtual ~EventManagerWrapper ();
            void loop_once(time_ms_t timeout);
            void loop();
            //void register_event(Channel* p_ch, CallBack&& cb);
            void register_event(uint32_t event, Channel* p_ch, CallBack&& cb);
            TimerId run_after(time_ms_t para_t, CallBack &&cb, time_ms_t interval = 0);    // after
            void exit();
            EventManager* get_pimpl();
        private:
            unique_ptr<EventManager> pimpl_;
    };

} /* my_http */ 

#endif /* ifndef EVENT_H */
