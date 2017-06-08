#include "threadpool.h"

namespace my_http {

ThreadPool::ThreadPool(int threadsize, int tasksize) : tasks_(tasksize), is_running_(true) {
    for (int i = 0; i < threadsize; i++) {
        thread_container_.emplace_back(std::thread([this]() {
            CallBack movetoit;
            bool one_more_task_done = false;
            while (is_running_ || !is_running_ && one_more_task_done) {
                auto check = tasks_.try_pop(std::ref(movetoit));
                if (check) {
                    if (!is_running_) { one_more_task_done = true; }
                    movetoit();
                } else {
                    // and wait;
                    std::unique_lock<std::mutex> lk(m_);
                    cv_.wait_for(lk, 5ms);
                }
            }
            return;
        }));
    }
}

ThreadPool::~ThreadPool() {
    std::call_once(once_, [this]() { stop(); });
}

void ThreadPool::stop() {
    tasks_.stop();
    is_running_ = false;
    for (auto& thread_ob : thread_container_) {
        thread_ob.join();
    }
    thread_container_.clear();
}

void ThreadPool::start() {
    if (!is_running_) {
        NOTDONE();
    } else {
        cv_.notify_all();
    }
}

bool ThreadPool::add_task(CallBack&& cb) {
    // blocking add
    auto check = tasks_.try_push(std::move(cb), 2);
    if (check) {

    } else {
        LOG_WARN("thread pool add task failed");
    }
    return check;
}

size_t ThreadPool::get_task_size() const { return tasks_size_; }
size_t ThreadPool::get_thread_size() const { return thread_size_; }

} /* my_http  */
