#include "Plugin/PluginManager.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

#include <MaaFramework/MaaAPI.h>
#include <filesystem>
#include <shared_mutex>

MAA_NS_BEGIN

static void phony_call_on_instance_created([[maybe_unused]] MaaInstanceHandle inst)
{
}

static void phony_call_on_instance_destroyed([[maybe_unused]] MaaInstanceHandle inst)
{
}

bool PluginHolder::load(std::filesystem::path path)
{
    LogFunc << VAR(path);

    module_.load(path);
    if (!module_.is_loaded()) {
        LogError << "Library load failed";
        return false;
    }
    if (!module_.has("maafw_plugin_init")) {
        LogError << "Find init symbol failed";
        module_.unload();
        return false;
    }
    reset_hook();

    auto init = module_.get<decltype(maafw_plugin_init)>("maafw_plugin_init");
    if (init && init(reinterpret_cast<MaaPluginHookDispatcher*>(&hook_))) {
        return true;
    }
    else {
        LogError << "Plugin init failed";
        module_.unload();
        return false;
    }
}

void PluginHolder::unload()
{
    if (module_.is_loaded()) {
        module_.unload();
    }
    reset_hook();
}

void PluginHolder::reset_hook()
{
    hook_ = { MaaPluginHookVersion_V0,
              phony_call_on_instance_created,
              phony_call_on_instance_destroyed };
}

void PluginManager::call_on_instance_created(MaaInstanceHandle inst)
{
    LogFunc;

    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& plugin : plugins_) {
        plugin->hook().on_instance_created(inst);
    }
}

void PluginManager::call_on_instance_destroyed(MaaInstanceHandle inst)
{
    LogFunc;

    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& plugin : plugins_) {
        plugin->hook().on_instance_destroyed(inst);
    }
}

void PluginManager::scan_load()
{
    LogFunc;

    std::unique_lock<std::shared_mutex> lock(mutex_);
    plugins_.clear();

    auto plugin_dir = library_dir() / "plugins";
    if (!std::filesystem::is_directory(plugin_dir)) {
        return;
    }
#if defined(_WIN32)
    auto lib_ext = ".dll";
#elif defined(__APPLE__)
    auto lib_ext = ".dylib";
#else
    auto lib_ext = ".so";
#endif
    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) {
            LogInfo << "Skip non regular file" << entry.path().filename();
            continue;
        }
        auto path = entry.path();
        auto ext = path_to_utf8_string(path.extension());
        if (ext != lib_ext) {
            LogInfo << "Skip non library" << entry.path().filename();
            continue;
        }
        auto holder = std::make_unique<PluginHolder>();
        if (holder->load(path)) {
            plugins_.push_back(std::move(holder));
        }
    }
}

MAA_NS_END
