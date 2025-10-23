#include "runtime.h"

#include <condition_variable>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "../apis/loader.h"
#include "../foundation/spec.h"

template <typename Type>
inline auto BindData(QuickJSRuntimeBridgeInterface::SinkFunc func, void* data)
{
    return [func, data](Type* handle, const char* msg, const char* details) {
        func(handle, msg, details, data);
    };
}

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

    std::vector<std::function<void(MaaResource*, const char*, const char*)>> plugin_resource_sinks;
    std::vector<std::function<void(MaaController*, const char*, const char*)>> plugin_controller_sinks;
    std::vector<std::function<void(MaaTasker*, const char*, const char*)>> plugin_tasker_sinks;
    std::vector<std::function<void(MaaContext*, const char*, const char*)>> plugin_context_sinks;

    QuickJSRuntimeData()
    {
        rt = JS_NewRuntime();
        ctx = JS_NewContext(rt);

        JS_SetDumpFlags(rt, JS_DUMP_LEAKS);
        JS_SetRuntimeOpaque(rt, this);
    }

    ~QuickJSRuntimeData()
    {
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }

    void load_modules()
    {
        // load globalThis.maa
        init_module_maa(ctx);

        // load sys
        init_module_sys(ctx);
    }

    bool has_running_task() { return running_task_count > 0; }

    bool has_pending_task_callback()
    {
        std::unique_lock lock(async_tasks_mtx);
        return async_tasks.size() > 0;
    }

    bool should_quit() { return !(JS_IsJobPending(rt) || has_running_task() || has_pending_task_callback()); }

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

        // transfer tasks to properly destruct values on main threads.

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

    virtual void plugin_add_sink(SinkType type, SinkFunc func, void* ptr) override
    {
        switch (type) {
        case QuickJSRuntimeBridgeInterface::Resource:
            plugin_resource_sinks.push_back(BindData<MaaResource>(func, ptr));
            break;
        case QuickJSRuntimeBridgeInterface::Controller:
            plugin_controller_sinks.push_back(BindData<MaaController>(func, ptr));
            break;
        case QuickJSRuntimeBridgeInterface::Tasker:
            plugin_tasker_sinks.push_back(BindData<MaaTasker>(func, ptr));
            break;
        case QuickJSRuntimeBridgeInterface::Context:
            plugin_context_sinks.push_back(BindData<MaaContext>(func, ptr));
            break;
        }
    }
};

QuickJSRuntime::QuickJSRuntime()
{
    d_ = new QuickJSRuntimeData;

    d_->load_modules();
}

QuickJSRuntime::~QuickJSRuntime()
{
    delete d_;
}

void QuickJSRuntime::eval_file(std::string path)
{
    std::ifstream fin(path);
    std::stringstream buf;
    buf << fin.rdbuf();

    std::string source = buf.str();
    eval_script(source);
}

void QuickJSRuntime::eval_script(std::string script)
{
    auto val = JS_Eval(d_->ctx, script.c_str(), script.size(), "index.js", JS_EVAL_FLAG_STRICT);
    JS_FreeValue(d_->ctx, val);
}

void QuickJSRuntime::exec_loop(bool auto_quit)
{
    JSContext* unused;
    while (true) {
        if (d_->quit) {
            break;
        }
        if (auto_quit && d_->should_quit()) {
            break;
        }
        while (true) {
            auto state = JS_ExecutePendingJob(d_->rt, &unused);
            if (!state) {
                break;
            }
            if (state < 0) {
                std::cerr << "got unhandled exception" << std::endl;
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

void QuickJSRuntime::dispatch_resource_sink(MaaResource* handle, const char* message, const char* details_json)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    d_->push_task([&promise, handle, message, details_json, &sinks = d_->plugin_resource_sinks](JSContext*) {
        // TODO: 这里实际上在串行执行所有sink，逻辑上用Promise.all并行会更好，但是复用的sink函数已经阻塞了。
        for (const auto& sink : sinks) {
            sink(handle, message, details_json);
        }
        promise.set_value();
    });
    return future.get();
}

void QuickJSRuntime::dispatch_controller_sink(MaaController* handle, const char* message, const char* details_json)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    d_->push_task([&promise, handle, message, details_json, &sinks = d_->plugin_controller_sinks](JSContext*) {
        for (const auto& sink : sinks) {
            sink(handle, message, details_json);
        }
        promise.set_value();
    });
    return future.get();
}

void QuickJSRuntime::dispatch_tasker_sink(MaaTasker* handle, const char* message, const char* details_json)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    d_->push_task([&promise, handle, message, details_json, &sinks = d_->plugin_tasker_sinks](JSContext*) {
        for (const auto& sink : sinks) {
            sink(handle, message, details_json);
        }
        promise.set_value();
    });
    return future.get();
}

void QuickJSRuntime::dispatch_context_sink(MaaContext* handle, const char* message, const char* details_json)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    d_->push_task([&promise, handle, message, details_json, &sinks = d_->plugin_context_sinks](JSContext*) {
        for (const auto& sink : sinks) {
            sink(handle, message, details_json);
        }
        promise.set_value();
    });
    return future.get();
}
