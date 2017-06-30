#ifndef MSG_RESPONSER_H
#define MSG_RESPONSER_H value
#include "facility.h"
#include "nettools.h"

namespace my_http {

using MsgResponserDoCallBack = std::function<void(Buffer&, Buffer&)>;

class MsgResponserInterface {
public:
    MsgResponserInterface();
    virtual ~MsgResponserInterface();
    virtual size_t get_require_size() = 0;
    virtual void do_it_for_con_of_seqno(uint32_t seqno, Buffer& rb, Buffer& wb) = 0;
    virtual void register_cb_for_con_of_seqno(uint32_t seqno, MsgResponserDoCallBack&& cb) = 0;

};

class EchoMsgResponser : public MsgResponserInterface {
    public:
        EchoMsgResponser();
        virtual ~EchoMsgResponser();
        size_t get_require_size() override;
        void do_it_for_con_of_seqno(uint32_t seqno, Buffer& rb, Buffer& wb) override;
        void register_cb_for_con_of_seqno(uint32_t seqno, MsgResponserDoCallBack&& cb) override;

private:
    size_t mini_size_ = 1;
    map<uint32_t, MsgResponserDoCallBack> cb_map_;
    MsgResponserDoCallBack echo_do_it; // this would be invoked if no cb for particular seqno
};

class MsgCodecInterface {
public:
    MsgCodecInterface ();
    virtual ~MsgCodecInterface ();
    virtual void to_decode(Buffer& buffer) = 0;
    virtual void to_encode(Buffer& buffer) = 0;
private:
    
};

} /* my_http  */
#endif /* ifndef MSG_RESPONSER_H */
