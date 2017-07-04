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

    // translate from Event to EventEnum in EventManagerWrapper::register_event()
    enum class EventEnum {IODemultiplexCB, IORead, IOWrite,
                        Timeout, PeerShutDown, Error};

    std::ostream& operator<<(std::ostream&, EventEnum);

    EventEnum uint2enum(uint32_t event);
    uint32_t enum2uint(EventEnum eventenum);

    class EventManager : private noncopyable {
        public:
            EventManager();
            virtual ~EventManager();
            //void register_event(Channel* p_ch, CallBack&& cb);
            void register_event(uint32_t event, Channel* p_ch, CallBack&& cb);
            void remove_registered_event(uint32_t event, Channel *p_ch);
            void remove_channel(Channel* p_ch);
            TimerId run_at(time_ms_t para_t, CallBack&& cb);    // means after FIXME
            TimerId run_after(time_ms_t para_t, CallBack&& cb);
            void loop();
            void loop_once(time_ms_t timeout);
            void add_active_event(EventEnum para_enum, Channel* p_ch);
            void handle_active_event();
            void start_up();
            void exit();
        private:
            // TODO we need a more safe and more effective key design
            map<pair<EventEnum, Channel*>, CallBack> event_call_back_map_;
            unique_ptr<IODemultiplexerInterface> io_demultiplexer_;
            vector<pair<EventEnum, Channel*>> active_channels_;
            unique_ptr<TimerQueue> my_timer_queue_;
            bool exit_;
            /* data */
    };

    class EventManagerWrapper : private noncopyable {
        public:
            EventManagerWrapper ();
            virtual ~EventManagerWrapper ();
            void loop_once(int timeout);
            void loop();
            //void register_event(Channel* p_ch, CallBack&& cb);
            void register_event(uint32_t event, Channel* p_ch, CallBack&& cb);
            TimerId run_at(time_ms_t para_t, CallBack&& cb);    // after
            void exit();
            EventManager* get_pimpl();
        private:
            unique_ptr<EventManager> pimpl_;
    };

} /* my_http */ 

#endif /* ifndef EVENT_H */
