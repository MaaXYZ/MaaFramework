#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "Common/Conf.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NonCopyable.hpp"

MAA_NS_BEGIN

template <typename Item>
class AsyncRunner : public NonCopyable
{
public:
    using Id = int64_t;
    using ProcessFunc = std::function<bool(Id id, Item item)>;
    using NotifyFunc = std::function<void(void)>;

    enum class Status
    {
        invalid = 0,
        pending = 1000,
        running = 2000,
        succeeded = 3000,
        failed = 4000,
    };

public:
    explicit AsyncRunner(ProcessFunc proc);
    virtual ~AsyncRunner();
    void release();

    Id post(Item item, bool block = false);
    void wait(Id id) const;
    void wait_all() const;
    Status status(Id id) const;

    void clear();
    bool running() const;

private:
    void working();

    ProcessFunc process_;

    std::list<std::pair<Id, Item>> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cond_;
    std::atomic_bool running_ = false;

    mutable std::shared_mutex status_mutex_;
    std::map<Id, Status> status_map_;

    Id compl_id_ = 0;
    mutable std::mutex compl_mutex_;
    mutable std::condition_variable compl_cond_;

    std::atomic_bool exit_ = false;
    inline static std::atomic<Id> cross_inst_id_ = kRunnerIdBase;

    std::thread thread_;
};

template <typename Item>
inline AsyncRunner<Item>::AsyncRunner(ProcessFunc proc)
    : process_(proc)
{
    // LogFunc;

    thread_ = std::thread(&AsyncRunner<Item>::working, this);
}

template <typename Item>
inline AsyncRunner<Item>::~AsyncRunner()
{
    // LogFunc;

    release();
}

template <typename Item>
inline void AsyncRunner<Item>::release()
{
    // LogFunc;

    exit_ = true;

    {
        std::unique_lock queue_lock(queue_mutex_);
        queue_cond_.notify_all();
    }

    {
        std::unique_lock compl_lock(compl_mutex_);
        compl_cond_.notify_all();
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

template <typename Item>
inline void AsyncRunner<Item>::working()
{
    // LogFunc;

    while (!exit_) {
        std::unique_lock queue_lock(queue_mutex_);
        if (exit_) {
            running_ = false;
            return;
        }

        if (queue_.empty()) {
            running_ = false;
            compl_cond_.notify_all();
            queue_cond_.wait(queue_lock);
            continue;
        }

        running_ = true;

        auto [id, item] = std::move(queue_.front());
        queue_.pop_front();
        queue_lock.unlock();

        std::unique_lock status_lock(status_mutex_);
        status_map_[id] = Status::running;
        status_lock.unlock();

        bool ret = process_(id, std::move(item));

        status_lock.lock();
        status_map_[id] = ret ? Status::succeeded : Status::failed;
        status_lock.unlock();

        std::unique_lock compl_lock(compl_mutex_);
        compl_id_ = id;
        compl_cond_.notify_all();
    }
}

template <typename Item>
inline typename AsyncRunner<Item>::Id AsyncRunner<Item>::post(Item item, bool block)
{
    // LogFunc;

    Id id = 0;
    {
        std::unique_lock queue_lock(queue_mutex_);
        id = ++cross_inst_id_;
        queue_.emplace_back(id, std::move(item));

        {
            std::unique_lock status_lock(status_mutex_);
            status_map_.emplace(id, Status::pending);
        }

        running_ = true;
        queue_cond_.notify_one();
    }

    if (block) {
        wait(id);
    }

    return id;
}

template <typename Item>
inline void AsyncRunner<Item>::wait(Id id) const
{
    // LogFunc << VAR(id);

    while (!exit_) {
        std::unique_lock compl_lock(compl_mutex_);
        if (exit_) {
            return;
        }

        if (id <= compl_id_) {
            return;
        }

        compl_cond_.wait(compl_lock);
    }
}

template <typename Item>
inline void AsyncRunner<Item>::wait_all() const
{
    LogFunc;

    while (!exit_) {
        std::unique_lock compl_lock(compl_mutex_);
        if (!running_ || exit_) {
            return;
        }

        compl_cond_.wait(compl_lock);
    }
}

template <typename Item>
inline AsyncRunner<Item>::Status AsyncRunner<Item>::status(Id id) const
{
    std::shared_lock status_lock(status_mutex_);

    auto iter = status_map_.find(id);
    if (iter == status_map_.end()) {
        return Status::invalid;
    }
    return iter->second;
}

template <typename Item>
inline void AsyncRunner<Item>::clear()
{
    // LogFunc;

    {
        std::unique_lock queue_lock(queue_mutex_);
        queue_ = {};
        queue_cond_.notify_all();
    }

    {
        std::unique_lock compl_lock(compl_mutex_);
        compl_id_ = cross_inst_id_;
        compl_cond_.notify_all();
    }

    {
        std::unique_lock status_lock(status_mutex_);
        status_map_.clear();
    }
}

template <typename Item>
inline bool AsyncRunner<Item>::running() const
{
    return running_;
}

MAA_NS_END
