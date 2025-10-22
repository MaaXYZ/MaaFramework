#include <thread>

#include <meojson/json.hpp>
#define MAA_PLUGIN_EXPORTS
#include <MaaPlugin/MaaPluginAPI.h>

#include "../utils/library.h"
#include "runtime.h"

static QuickJSRuntime* runtime {};

static QuickJSRuntime* getRuntime()
{
    if (!runtime) {
        runtime = new QuickJSRuntime;

        auto folder = get_library_path(reinterpret_cast<void*>(getRuntime)).parent_path();
        auto json_path = folder.append("MaaQjsConfig.json");
        auto config = json::open(json_path).value_or(
            json::object {
                { "scripts", json::array {} },
            });
        auto scripts = config["scripts"].as_array();
        std::vector<std::string> scriptPaths;
        for (auto file : scripts) {
            if (file.is_string()) {
                scriptPaths.push_back(folder.append(file.as_string()).string());
            }
        }

        std::thread { [scriptPaths]() {
            for (auto path : scriptPaths) {
                runtime->eval_file(path);
            }
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
    getRuntime();

    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnControllerEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    getRuntime();

    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnTaskerEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    getRuntime();

    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

void OnContextEvent(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    getRuntime();

    std::ignore = handle;
    std::ignore = message;
    std::ignore = details_json;
    std::ignore = trans_arg;
}

