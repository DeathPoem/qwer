#include "msg_responser.h"

namespace my_http {
    
    MsgResponser::MsgResponser() {}

    MsgResponser::~MsgResponser() {}

    size_t MsgResponser::get_require_size() {
        return mini_size_;
    }

    void MsgResponser::set_mini_require_size(size_t mini_size) {
        mini_size_ = mini_size;
    }

    void MsgResponser::do_it_for_con_of_seqno(uint32_t seqno, Buffer& rb, Buffer& wb) {
        auto found = cb_map_.find(seqno);
        if (found != cb_map_.end()) {
            get<1>(*found)(rb, wb);
        } else {
            echo_do_it(rb, wb);
        }
    }

    void MsgResponser::register_cb_for_con_of_seqno(uint32_t seqno, MsgResponserDoCallBack&& cb) {
        cb_map_[seqno] = std::move(cb);
    }
} /* my_http  */ 
