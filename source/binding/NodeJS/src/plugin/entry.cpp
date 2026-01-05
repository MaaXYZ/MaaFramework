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

const char* GetPluginVersion(void)
{
    return MAA_VERSION;
}

uint32_t GetApiVersion(void)
{
    return 1;
}

void OnResourceEvent(void* handle, const char* message, const char* details_json, void*)
{
    getRuntime()->dispatch_resource_sink(reinterpret_cast<MaaResource*>(handle), message, details_json);
}

void OnControllerEvent(void* handle, const char* message, const char* details_json, void*)
{
    getRuntime()->dispatch_controller_sink(reinterpret_cast<MaaController*>(handle), message, details_json);
}

void OnTaskerEvent(void* handle, const char* message, const char* details_json, void*)
{
    getRuntime()->dispatch_tasker_sink(reinterpret_cast<MaaTasker*>(handle), message, details_json);
}

void OnContextEvent(void* handle, const char* message, const char* details_json, void*)
{
    getRuntime()->dispatch_context_sink(reinterpret_cast<MaaContext*>(handle), message, details_json);
}

