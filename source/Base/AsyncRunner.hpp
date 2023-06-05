#pragma once

#include "Common/MaaConf.h"
#include "Utils/Logger.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

MAA_NS_BEGIN

template <typename Item>
class AsyncRunner
{
public:
    using Id = int64_t;
    using ProcessFunc = std::function<void(Id id, Item item)>;

public:
    AsyncRunner(ProcessFunc run_task);
    AsyncRunner(const AsyncRunner&) = delete;
    AsyncRunner(AsyncRunner&&) = delete;
    virtual ~AsyncRunner();

    AsyncRunner& operator=(const AsyncRunner&) = delete;
    AsyncRunner& operator=(AsyncRunner&&) = delete;

    Id post(Item item, bool block = false);
    bool wait(Id id);

    void clear();
    bool running() const;

private:
    void working();

    ProcessFunc process_;

    std::queue<std::pair<Id, Item>> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread thread_;
    std::atomic_bool running_ = false;

    Id compl_id_ = 0;
    std::mutex compl_mutex_;
    std::condition_variable compl_cond_;

    std::atomic_bool exit_ = false;
    inline static Id cross_inst_id_ = 0;
};

template <typename Item>
inline AsyncRunner<Item>::AsyncRunner(ProcessFunc run_task) : process_(run_task)
{
    LogFunc;

    thread_ = std::thread(&AsyncRunner<Item>::working, this);
}

template <typename Item>
inline AsyncRunner<Item>::~AsyncRunner()
{
    LogFunc;

    exit_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(compl_mutex_);
        compl_cond_.notify_all();
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

template <typename Item>
inline void AsyncRunner<Item>::working()
{
    LogFunc;

    while (!exit_) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            running_ = false;
            cond_.wait(lock);
            continue;
        }

        running_ = true;

        auto [id, item] = std::move(queue_.front());
        queue_.pop();
        lock.unlock();

        process_(id, std::move(item));

        std::unique_lock<std::mutex> compl_lock(compl_mutex_);
        compl_id_ = id;
        compl_cond_.notify_all();
    }
}

template <typename Item>
inline AsyncRunner<Item>::Id AsyncRunner<Item>::post(Item item, bool block)
{
    LogFunc;

    Id id = MaaInvalidId;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        id = ++cross_inst_id_;
        queue_.emplace(id, std::move(item));
        cond_.notify_one();
    }

    if (block) {
        wait(id);
    }

    return id;
}

template <typename Item>
inline bool AsyncRunner<Item>::wait(Id id)
{
    LogFunc << VAR(id);

    while (!exit_) {
        std::unique_lock<std::mutex> lock(compl_mutex_);
        if (id <= compl_id_) {
            return true;
        }

        compl_cond_.wait(lock);
    }
    return false;
}

template <typename Item>
inline void AsyncRunner<Item>::clear()
{
    LogFunc;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_ = {};
        cond_.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(compl_mutex_);
        compl_id_ = cross_inst_id_;
        compl_cond_.notify_all();
    }
}

template <typename Item>
inline bool AsyncRunner<Item>::running() const
{
    return running_;
}

MAA_NS_END
