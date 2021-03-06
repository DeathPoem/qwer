#ifndef MULTITHREAD_VERSION_H
#define MULTITHREAD_VERSION_H value
#include "facility.h"
#include "tcpserver.h"
#include "threadpool.h"

namespace my_http {

    enum class MultithreadTag {SingleThread, Multithread, Invalid,};

template <typename T>
class AcceptedQueue : private SyncQueue<T> {  //!< why use private :https://stackoverflow.com/questions/656224/when-should-i-use-c-private-inheritance/675451#675451
public:
    AcceptedQueue(size_t workersize = ThreadPool::get_default_threadpool_size() - 1); //!< minus one is because one thread would be acceptor_routine
    virtual ~AcceptedQueue();
    bool push_one(T&& one);
    vector<T> get_one();

private:
    atomic<size_t> cur_;
    const size_t workersize_;
    mutex xm_;
    size_t get_once_amount() const;
};
 
template <typename T>
    AcceptedQueue<T>::AcceptedQueue(size_t workersize) : 
        workersize_(workersize), 
        cur_(0),
        SyncQueue<T>() {

    }

template <typename T>
    AcceptedQueue<T>::~AcceptedQueue() {}

template <typename T>
    bool AcceptedQueue<T>::push_one(T&& one) {
        cur_++;
        if (SyncQueue<T>::try_push(move(one), 5)) {
            return true;
        } else {
            NOTDONE();
        }
    }

template <typename T>
    vector<T> AcceptedQueue<T>::get_one() {
        std::lock_guard<mutex> lk(xm_);
        size_t amount = get_once_amount();
        T movetoit;
        vector<decltype(movetoit)> toreturn;
        for (int i = 0; i < amount; i++) {
            SyncQueue<T>::try_pop(movetoit);
            toreturn.push_back(move(movetoit));
        }
        cur_ -= toreturn.size();
        return toreturn;
    }

template <typename T>
    size_t AcceptedQueue<T>::get_once_amount() const {
        return cur_ > 0 ? (cur_ / workersize_) + 1 : 0;
    }

using MoveFDCallBack = std::function<void(pair<FileDescriptorType, Ipv4Addr>&&)>;
namespace detail {
    
//! @brief an oop principle broke case, but I still doing so for convenience. A more elegant and orthodox method is to abstract a base class for MultiAcceptor and SingleThreadAcceptor to derive.
class MultiAcceptorImp : public Acceptor {
public:
    MultiAcceptorImp(EventManagerWrapper* emwp);
    virtual ~MultiAcceptorImp ();
    MultiAcceptorImp& set_accept_readable_callback(MoveFDCallBack&& cb);
    void epoll_and_accept(time_ms_t after = 0) override;

private:
    void listen_it_and_accept() override;
    /* data */
    void handle_epoll_readable() override;
    MoveFDCallBack movecb_x_;
};

} /* detail */ 

//! @brief wrapped it with good
class MultiAcceptor : private detail::MultiAcceptorImp {
public:
    MultiAcceptor (EventManagerWrapper* emwp);
    virtual ~MultiAcceptor ();
    // TODO add interface 
    MultiAcceptor& set_listen_addr(Ipv4Addr addr);
    MultiAcceptor& set_accept_readable_callback(MoveFDCallBack&& cb);
    void epoll_and_accept(time_ms_t after = 0);
    //TCPSTATE get_state();
    using MultiAcceptorImp::get_state;
private:
    /* data */
};

class MultiServer { //!< a multithread safe class
public:
    MultiServer (size_t idle_duration = 100, Ipv4Addr listen_ip = Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8001));
    virtual ~MultiServer ();
    void AcceptorRoutine(); //!< to guanrantee MsgServerRoutineDetail invoke once
    void MsgServerRoutine();
    MultiServer& set_msg_responser_callback(MsgResponserCallBack&& cb);
    MultiServer& set_tcp_callback(TCPCallBack && cb);
    //MultiServer& set_after_to_write_cb(TCPCallBack&& cb);
    void ThreadPoolStart(function<bool()>&& block_checker = nullptr);
    void Exit();
private:
    std::function<void()> get_fetch_routine(unordered_map<size_t, shared_ptr<TCPConnection>>& tcpcon_map, EventManagerWrapper& emwp, size_t& seqno);
    void MsgServerRoutineDetail();    //!< we would put event.exit into stop_vec
    void AcceptorRoutineDetail();     //!< we would put event.exit into stop_vec
    bool FetchOne(vector<pair<FileDescriptorType, Ipv4Addr>>& toit);
    const Ipv4Addr listen_ip_;
    const size_t idle_duration_;
    ThreadPool thread_pool_;
    atomic<bool> is_accepting_;
    mutex syncmutex_;
    condition_variable synccv_;
    using Qtype = pair<FileDescriptorType, Ipv4Addr>;
    unique_ptr<AcceptedQueue<Qtype>> p_queue_;
    MsgResponserCallBack msg_responser_cb_;
    //TCPCallBack after_to_write_cb_;
    TCPCallBack tcp_cb_;
    vector<CallBack> pool_stop_cb_vec_;
    bool threads_in_pool_ends_ = false;
    CallBack pool_stop_cb_; //!< a caller invoker cb in stop_vec
    function<bool()> default_current_thread_block_check_;   //!< if return true, would continue block current thread
    condition_variable cur_block_cv_;
    mutex cur_block_mutex_;
};

} /* my_http */

#endif /* ifndef MULTITHREAD_VERSION_H */
