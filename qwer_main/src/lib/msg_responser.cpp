#include "msg_responser.h"

namespace my_http {

    MsgResponserInterface::MsgResponserInterface(){ }

    MsgResponserInterface::~MsgResponserInterface(){}

    EchoMsgResponser::EchoMsgResponser(){
        echo_do_it = [](Buffer& rb, Buffer& wb) {
            char readto[4000];
            if (rb.get_readable_bytes() >= 4000) {
                NOTDONE();
            }
            memset(readto, 0, sizeof readto);
            int require_size = rb.get_readable_bytes();
            LOG_DEBUG("in echo, read %d", require_size);
            rb.read_from_buffer(readto, require_size);
            auto consume_size = strlen(readto);
            LOG_DEBUG(" bad ! readto is %s", readto);
            rb.consume(consume_size);
            wb.write_to_buffer(readto, consume_size);
            LOG_DEBUG(" ! %d", wb.get_readable_bytes());
        };
    }

    EchoMsgResponser::~EchoMsgResponser(){}

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
