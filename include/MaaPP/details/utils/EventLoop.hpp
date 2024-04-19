#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

namespace maa::utils
{

class EventLoop
{
public:
    void defer(std::function<void()> func)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        task_.push_back(std::move(func));
        cond_.notify_all();
    }

    void bind() { current_.push_back(this); }

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
        current_.pop_back();
    }

    void stop() { running_ = false; }

    static auto current() { return current_.back(); }

private:
    static inline std::vector<EventLoop*> current_;

    std::mutex mtx_;
    std::condition_variable cond_;
    std::vector<std::function<void()>> task_;
    bool running_ = false;
};

}
