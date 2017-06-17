#include "msg_responser.h"

namespace my_http {
    
    EchoMsgResponser::EchoMsgResponser() {
        echo_do_it = [](Buffer& rb, Buffer wb) {
            char readto[4000];
            if (rb.get_readable_bytes() >= 4000) {
                NOTDONE();
            }
            rb.read_from_buffer(readto, rb.get_readable_bytes());
            auto consume_size = strlen(readto);
            rb.consume(consume_size);
            char result[4000];
            wb.write_to_buffer(result, consume_size);
        };  // this would be invoked if no cb for particular seqno
    }

    EchoMsgResponser::~EchoMsgResponser() {}

    size_t EchoMsgResponser::get_require_size() {
        return mini_size_;
    }

    void EchoMsgResponser::do_it_for_con_of_seqno(uint32_t seqno, Buffer& rb, Buffer& wb) {
        auto found = cb_map_.find(seqno);
        if (found != cb_map_.end()) {
            get<1>(*found)(rb, wb);
        } else {
            echo_do_it(rb, wb);
        }
    }

    void EchoMsgResponser::register_cb_for_con_of_seqno(uint32_t seqno, MsgResponserDoCallBack&& cb) {
        //cb_map_[seqno] = std::move(cb);   // nothing would be done with Echo
    }
} /* my_http  */ 
