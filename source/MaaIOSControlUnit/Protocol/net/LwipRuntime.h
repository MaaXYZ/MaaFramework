#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

namespace maa::ios::net
{

// 进程内唯一的 lwIP 核心线程，串行执行 raw API、协议回调和定时器。
// 提交的任务应只完成短操作，不能做隧道 I/O，也不能等待应用消费数据。
// TCP 连接、发送空间及收包等待由应用线程承担，避免阻塞其它端点和定时器。
class LwipRuntime
{
public:
    static LwipRuntime& instance();

    template <typename Fn>
    auto call(Fn fn) -> decltype(fn())
    {
        // 核心线程中的回调可能继续调用端点操作；直接执行，避免排队后等待自身。
        if (std::this_thread::get_id() == worker_.get_id()) {
            return fn();
        }
        auto task = std::make_shared<std::packaged_task<decltype(fn())()>>(std::move(fn));
        auto result = task->get_future();
        {
            std::unique_lock lock(mutex_);
            // 队列满时在调用线程等待空间；入队后释放队列锁，再等待核心操作的结果。
            cv_.wait(lock, [this] { return tasks_.size() < 1024; });
            tasks_.emplace_back([task] { (*task)(); });
        }
        cv_.notify_all();
        return result.get();
    }

    unsigned char close_arg_id() const { return close_arg_id_; }

private:
    LwipRuntime();
    ~LwipRuntime();
    void run(std::promise<void> ready);
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;
    bool stopping_ = false;
    unsigned char close_arg_id_ = 0;
    std::thread worker_;
};
} // namespace maa::ios::net
