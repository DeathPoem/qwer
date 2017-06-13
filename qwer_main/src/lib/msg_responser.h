#ifndef MSG_RESPONSER_H
#define MSG_RESPONSER_H value
#include "facility.h"
#include "nettools.h"

namespace my_http {

using MsgResponserDoCallBack = std::function<void(Buffer&, Buffer&)>;

class MsgResponser {
public:
    MsgResponser();
    virtual ~MsgResponser();
    size_t get_require_size();
    void set_mini_require_size(size_t mini_size);
    void do_it_for_con_of_seqno(uint32_t seqno, Buffer& rb, Buffer& wb);
    void register_cb_for_con_of_seqno(uint32_t seqno, MsgResponserDoCallBack&& cb);

private:
    size_t mini_size_ = 1;
    map<uint32_t, MsgResponserDoCallBack> cb_map_;
    MsgResponserDoCallBack echo_do_it; // this would be invoked if no cb for particular seqno
};
} /* my_http  */
#endif /* ifndef MSG_RESPONSER_H */
