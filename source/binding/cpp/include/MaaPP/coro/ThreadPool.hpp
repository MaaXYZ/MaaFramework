// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace maa::coro
{

class ThreadPool
{
public:
    ThreadPool(size_t count = 8)
    {
        threads_.reserve(count);
        for (size_t i = 0; i < count; i++) {
            threads_.emplace_back([this]() { this->run(); });
        }
    }

    ~ThreadPool()
    {
        stop();
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    template <typename F>
    void defer(F f)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks_.push(std::move(f));
        slave_cond_.notify_one();
    }

    void wait_all()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (tasks_.size() > 0) {
            master_cond_.wait(lock);
        }
    }

    void stop()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        running_ = false;
        slave_cond_.notify_all();
        quit_cond_.notify_all();
    }

    void exec()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        quit_cond_.wait(lock, [this]() { return !this->running_; });
    }

private:
    void run()
    {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            if (!running_) {
                return;
            }
            if (tasks_.size() == 0) {
                slave_cond_.wait(lock, [this]() { return tasks_.size() > 0 || !running_; });
            }
            if (!running_) {
                return;
            }
            auto task = std::move(tasks_.front());
            tasks_.pop();
            if (tasks_.size() == 0) {
                master_cond_.notify_all();
            }
            lock.unlock();

            task();
        }
    }

    bool running_ = true;
    std::vector<std::thread> threads_;

    std::queue<std::function<void()>> tasks_;

    std::mutex mtx_;
    std::condition_variable slave_cond_;
    std::condition_variable master_cond_;
    std::condition_variable quit_cond_;
};

}
