#include "QuickJSRuntime.h"

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

#define MAA_JS_IMPL_IS_QUICKJS
#include "../../binding/NodeJS/src/foundation/spec.h"

void init_module_maa(JSContext* ctx);
void init_module_sys(JSContext* ctx);

MAA_TOOLKIT_NS_BEGIN

struct QuickJSRuntimeData : public QuickJSRuntimeBridgeInterface
{
    JSRuntime* rt {};
    JSContext* ctx {};
    bool quit {};
    std::string result;

    std::mutex running_task_count_mtx;
    std::condition_variable running_task_count_cv;
    size_t running_task_count {};

    std::mutex async_tasks_mtx;
    std::condition_variable async_tasks_cv;
    std::vector<std::function<void(JSContext* ctx)>> async_tasks;

    QuickJSRuntimeData()
    {
        rt = JS_NewRuntime();
        ctx = JS_NewContext(rt);

        JS_SetDumpFlags(rt, JS_DUMP_LEAKS);

        // load globalThis.maa
        init_module_maa(ctx);

        // load sys
        init_module_sys(ctx);
    }

    ~QuickJSRuntimeData()
    {
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }

    bool has_running_task() { return running_task_count > 0; }

    bool has_pending_task_callback()
    {
        std::unique_lock lock(async_tasks_mtx);
        return async_tasks.size() > 0;
    }

    std::vector<std::function<void(JSContext* ctx)>> take_task_callbacks()
    {
        std::vector<std::function<void(JSContext * ctx)>> ret_tasks;
        std::unique_lock lock(async_tasks_mtx);
        std::swap(async_tasks, ret_tasks);
        return ret_tasks;
    }

    void wait_task_callback()
    {
        std::unique_lock lock(async_tasks_mtx);
        async_tasks_cv.wait(lock, [this]() { return async_tasks.size() > 0 || quit; });
    }

    void wait_running_task()
    {
        std::unique_lock lock(running_task_count_mtx);
        running_task_count_cv.wait(lock, [this]() { return running_task_count == 0; });
    }

    void reg_task() override
    {
        std::unique_lock lock(running_task_count_mtx);
        ++running_task_count;
    }

    void push_task(std::function<void(JSContext* ctx)> task) override
    {
        bool notify_running_task_done = false;
        {
            std::unique_lock lock(running_task_count_mtx);
            --running_task_count;
            if (running_task_count == 0) {
                notify_running_task_done = true;
            }
        }

        // 这里即使是quit状态，也要存起来所有task
        // 这样其它线程的worker才能把所有权移交到主线程
        // 直接在这里丢弃的话，会导致在其它线程析构
        {
            std::unique_lock lock(async_tasks_mtx);
            async_tasks.push_back(std::move(task));
        }
        async_tasks_cv.notify_all();

        if (notify_running_task_done) {
            running_task_count_cv.notify_all();
        }
    }

    virtual void call_exit(std::string ret) override
    {
        quit = true;
        result = ret;
        async_tasks_cv.notify_all();
    }
};

QuickJSRuntime::QuickJSRuntime()
{
    d_ = new QuickJSRuntimeData;
    JS_SetRuntimeOpaque(d_->rt, d_);
}

QuickJSRuntime::~QuickJSRuntime()
{
    delete d_;
}

void QuickJSRuntime::eval_script(std::string script)
{
    auto val = JS_Eval(d_->ctx, script.c_str(), script.size(), "index.js", JS_EVAL_FLAG_STRICT);
    JS_FreeValue(d_->ctx, val);
}

void QuickJSRuntime::exec_loop()
{
    JSContext* unused;
    while ((JS_IsJobPending(d_->rt) || d_->has_pending_task_callback() || d_->has_running_task()) && !d_->quit) {
        while (true) {
            auto state = JS_ExecutePendingJob(d_->rt, &unused);
            if (!state) {
                break;
            }
            if (state < 0) {
                std::cerr << "got exception" << std::endl;
                d_->quit = true;
                return;
            }
        }

        auto tasks = d_->take_task_callbacks();
        for (const auto& task : tasks) {
            task(d_->ctx);
        }

        if (!JS_IsJobPending(d_->rt)) {
            d_->wait_task_callback();
        }
    }

    d_->wait_running_task();
    d_->take_task_callbacks();
}

std::string QuickJSRuntime::get_result()
{
    return d_->result;
}

MAA_TOOLKIT_NS_END
