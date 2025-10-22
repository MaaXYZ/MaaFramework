#include <thread>

#define MAA_PLUGIN_EXPORTS
#include <MaaPlugin/MaaPluginAPI.h>

#include "runtime.h"

static QuickJSRuntime* runtime {};

QuickJSRuntime* getRuntime()
{
    if (!runtime) {
        runtime = new QuickJSRuntime;

        std::thread { []() {
            runtime->eval_script("");
            runtime->exec_loop(false);
        } }.detach();
    }

    return runtime;
}

uint32_t GetApiVersion()
{
    return 1;
}

void OnResourceEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnControllerEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnTaskerEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnContextEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

