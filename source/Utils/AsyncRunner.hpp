#pragma once

#include "Common/MaaConf.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

MAA_NS_BEGIN

using AsyncCallId = int;

extern std::atomic<AsyncCallId> __call_id;

template<typename Item>
class AsyncRunner
{
    AsyncRunner(std::function<void(AsyncCallId id, Item item)> process);
    AsyncRunner(const AsyncRunner&) = delete;
    AsyncRunner(AsyncRunner&&) = delete;
    virtual ~AsyncRunner();

    AsyncRunner& operator=(const AsyncRunner&) = delete;
    AsyncRunner& operator=(AsyncRunner&&) = delete;

    void run();
    AsyncCallId call(Item item, bool block = false);
    bool wait(AsyncCallId id);

private:
    std::function<void(AsyncCallId id, Item item)> process_;

    std::queue<std::pair<AsyncCallId, Item>> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread thread_;

    AsyncCallId compl_id_ = 0;
    std::mutex compl_mutex_;
    std::condition_variable compl_cond_;

    bool exit_ = false;
};


template <typename Item>
inline AsyncRunner<Item>::AsyncRunner(std::function<void(AsyncCallId id, Item item)> process) : process_(process)
{
    thread_ = std::thread(&Assistant::run, this);
}

template <typename Item>
inline AsyncRunner<Item>::~AsyncRunner()
{
    LogTraceFunction;

    exit_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

template <typename Item>
inline void AsyncRunner<Item>::run()
{
    LogTraceFunction;

    while (!exit_) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            cond_.wait(lock);
            continue;
        }

        auto [id, item] = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        
        process_(id, std::move(item));

        std::unique_lock<std::mutex> lock(compl_mutex_);
        compl_id_ = id;
        compl_cond_.notify_all();
    }
}

template <typename Item>
inline AsyncCallId AsyncRunner<Item>::call(Item item, bool block)
{
    LogTraceFunction;

    AsyncCallId id = 0;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        id = ++__call_id;
        queue_.emplace(id, std::move(item));
        cond_.notify_one();
    }
    if (block) {
        
    }

    return id;
}

template <typename Item>
inline bool AsyncRunner<Item>::wait(AsyncCallId id)
{
    while (!exit_) {
        std::unique_lock<std::mutex> lock(compl_mutex_);
        if (id <= compl_id_) {
            return true;
        }
        compl_cond_.wait(lock);
    }
    return false;
}

MAA_NS_END
