#ifndef THREADPOOL_H
#define THREADPOOL_H value

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "facility.h"

namespace my_http {

    //! @note actually, we can use SyncQueue
    // TODO delete this
template<typename T>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue ();
        ThreadSafeQueue(ThreadSafeQueue const & other);
        ThreadSafeQueue& operator=(ThreadSafeQueue const & other) = delete;
        virtual ~ThreadSafeQueue ();
    
        void push(T new_v);
        bool try_pop_2(T& value);
        void wait_until_pop_2(T& value);
        bool empty() const;
    private:
        std::mutex m_;
        std::queue<T> queue_;

    };

//! @brief A thread safe queue
template <typename TaskT>
class SyncQueue {
public:
    SyncQueue(int tasksize = 1000);
    virtual ~SyncQueue();
    // would try wait for timeout
    bool try_push(TaskT&& t, int millisec);
    bool try_pop(TaskT& movetoit);
    bool stop();
    bool is_empty() const;
    bool is_full() const;
    size_t get_cur_size() const;

private:
    size_t task_size_;
    bool stopflag_;
    std::queue<TaskT> task_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

using std::chrono_literals::operator""ms;

class ThreadPool {
public:
    //! @brief the init behavior is every threads in that threadpool wait for cv_ until start().
    ThreadPool(int threadsize = ThreadPool::get_default_threadpool_size(), int tasksize = 1000);
    virtual ~ThreadPool();
    //! @brief notify every threads to wake up and work, won't block the current thread, user should block the current thread if threads in pool depends on resources in current thread.
    void start();
    void stop_w();
    bool add_task(CallBack&& cb);
    size_t get_task_size() const;
    size_t get_thread_size() const;
    static size_t get_default_threadpool_size();
private:
    void stop();
    SyncQueue<CallBack> tasks_;
    std::vector<std::thread> thread_container_;
    std::atomic_bool is_running_;
    std::once_flag once_;
    std::mutex m_;
    size_t tasks_size_;
    size_t thread_size_;
    std::condition_variable cv_;
};

template <typename TaskT>
SyncQueue<TaskT>::SyncQueue(int tasksize) : task_size_(tasksize) {}

template <typename TaskT>
SyncQueue<TaskT>::~SyncQueue() {}

template <typename TaskT>
size_t SyncQueue<TaskT>::get_cur_size() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return task_queue_.size();
    ;
}

template <typename TaskT>
bool SyncQueue<TaskT>::is_empty() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return task_queue_.empty();
}

template <typename TaskT>
bool SyncQueue<TaskT>::try_pop(TaskT& movetoit) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!task_queue_.empty()) {
        swap(movetoit, task_queue_.front());
        task_queue_.pop();
        return true;
    } else {
        return false;
    }
}

template <typename TaskT>
bool SyncQueue<TaskT>::stop() {
    std::lock_guard<std::mutex> lk(mutex_);
    stopflag_ = true;
}

template <typename TaskT>
bool SyncQueue<TaskT>::try_push(TaskT&& t, int millisec) {
    std::unique_lock<std::mutex> lk(mutex_);
    int check = cv_.wait_for(lk, millisec * 1ms, [this]() {
        return task_queue_.size() < task_size_;
    });
    if (!check) {
        LOG_WARN("try push in SyncQueue, timeout");
    } else {
        if (task_queue_.size() < task_size_) {
            task_queue_.push(std::move(t));
            return true;
        } else {
            return false;
        }
    }
}

template <typename TaskT>
bool SyncQueue<TaskT>::is_full() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return task_queue_.size() == task_size_;
}
} /* my_http */

#endif /* ifndef THREADPOOL_H */
