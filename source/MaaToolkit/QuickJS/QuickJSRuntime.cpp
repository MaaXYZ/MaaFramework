#include "QuickJSRuntime.h"

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

#define MAA_JS_IMPL_IS_QUICKJS
#include "../../binding/NodeJS/src/foundation/bridge.h"

void init_module_maa(JSContext* ctx);
void init_module_sys(JSContext* ctx);

MAA_TOOLKIT_NS_BEGIN

struct QuickJSRuntimeData : public QuickJSRuntimeBridgeInterface
{
    JSRuntime* rt;
    JSContext* ctx;
    bool quit;
    std::string result;

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

    std::vector<std::function<void(JSContext* ctx)>> take_tasks()
    {
        std::vector<std::function<void(JSContext * ctx)>> result;
        std::unique_lock lock(async_tasks_mtx);
        std::swap(async_tasks, result);
        return result;
    }

    void wait_task()
    {
        std::unique_lock lock(async_tasks_mtx);
        async_tasks_cv.wait(lock, [this]() { return async_tasks.size() > 0 || quit; });
    }

    void push_task(std::function<void(JSContext* ctx)> task) override
    {
        {
            std::unique_lock lock(async_tasks_mtx);
            async_tasks.push_back(std::move(task));
        }
        async_tasks_cv.notify_all();
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
    while (JS_IsJobPending(d_->rt) || !d_->quit) {
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

        auto tasks = d_->take_tasks();
        for (const auto& task : tasks) {
            task(d_->ctx);
        }

        if (!JS_IsJobPending(d_->rt)) {
            d_->wait_task();
        }
    }
}

std::string QuickJSRuntime::get_result()
{
    return d_->result;
}

MAA_TOOLKIT_NS_END
