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
template <typename TaskT>
class SyncQueue {
public:
    SyncQueue(int tasksize);
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
    ThreadPool(int threadsize, int tasksize);
    virtual ~ThreadPool();
    void start();
    void stop();
    bool add_task(CallBack&& cb);
    size_t get_task_size() const;
    size_t get_thread_size() const;

private:
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
