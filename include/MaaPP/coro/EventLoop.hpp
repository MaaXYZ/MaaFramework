#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

namespace maa::coro
{

class EventLoop
{
public:
    EventLoop() { current_ = this; }

    ~EventLoop() { current_ = nullptr; }

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void defer(std::function<void()> func)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        task_.push_back(std::move(func));
        cond_.notify_all();
    }

    void defer_stop()
    {
        defer([this]() { this->stop(); });
    }

    void exec()
    {
        running_ = true;
        while (running_) {
            std::vector<std::function<void()>> tasks;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cond_.wait(lock, [this]() { return task_.size() > 0; });
                tasks.swap(task_);
            }
            for (const auto& func : tasks) {
                func();
            }
        }
    }

    void stop() { running_ = false; }

    static EventLoop* current() { return current_; }

private:
    static inline EventLoop* current_ = nullptr;

    std::mutex mtx_;
    std::condition_variable cond_;
    std::vector<std::function<void()>> task_;
    bool running_ = false;
};

}
