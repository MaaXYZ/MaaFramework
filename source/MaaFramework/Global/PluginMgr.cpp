#include "PluginMgr.h"

#include <ranges>

#include "MaaFramework/MaaAPI.h"
#include "MaaPlugin/MaaPluginAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/Runtime.h"

MAA_GLOBAL_NS_BEGIN

struct DefaultPluginsLoader
{
    DefaultPluginsLoader()
    {
        auto plugins_dir = library_dir() / "plugins";
        std::error_code ec;
        if (std::filesystem::is_directory(plugins_dir, ec)) {
            PluginMgr::get_instance().load(plugins_dir);
        }
    }
} _;

bool PluginMgr::load(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    std::lock_guard lock(mutex_);
    if (instances_ || shutdown_ || managing_) {
        LogError << "Load plugins before creating framework objects";
        return false;
    }
    if (std::filesystem::is_directory(library_path)) {
        bool ret = false;
        for (auto& entry : std::filesystem::recursive_directory_iterator(library_path)) {
            ret |= load_and_parse(std::filesystem::absolute(entry.path()));
        }

        return ret;
    }
    else {
        return load_and_parse(library_dir() / library_path) || load_and_parse(library_path);
    }
}

std::optional<boost::dll::shared_library> PluginMgr::load_dll(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    if (auto it = plugins_.find(path_to_utf8_string(library_path)); it != plugins_.end()) {
        LogDebug << "Library already loaded" << VAR(library_path);
        return it->second.library;
    }

    boost::dll::shared_library library;
    boost::dll::fs::error_code ec;
    library.load(library_path, ec, boost::dll::load_mode::append_decorations | boost::dll::load_mode::search_system_folders);

    if (ec) {
        LogError << "Failed to load library" << VAR(library_path) << VAR(ec.message());
        return std::nullopt;
    }

    if (!library.is_loaded()) {
        LogError << "Module is not loaded" << VAR(library_path);
        return std::nullopt;
    }

    return library;
}

bool PluginMgr::load_and_parse(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    auto opt = load_dll(library_path);
    if (!opt) {
        LogError << "Failed to load DLL" << VAR(library_path);
        return false;
    }

    auto& library = *opt;
    std::filesystem::path filepath = library.location();
    LogInfo << VAR(library_path) << VAR(filepath);
    if (plugins_.contains(path_to_utf8_string(filepath))) {
        return true;
    }

    constexpr const char* kFuncGetPluginVersion = "GetPluginVersion";
    if (library.has(kFuncGetPluginVersion)) {
        auto func = library.get<decltype(GetPluginVersion)>(kFuncGetPluginVersion);
        std::string_view plugin_version = func();
        LogInfo << VAR(plugin_version) << VAR(filepath);
    }
    else {
        LogWarn << "No func, ignore" << kFuncGetPluginVersion << VAR(filepath);
    }

    constexpr uint32_t kApiVersion = 1;
    constexpr const char* kFuncGetApiVersion = "GetApiVersion";
    if (library.has(kFuncGetApiVersion)) {
        auto func = library.get<decltype(GetApiVersion)>(kFuncGetApiVersion);
        if (!func) {
            LogError << "Failed to get function" << VAR(kFuncGetApiVersion) << VAR(filepath);
            return false;
        }
        uint32_t api_version = func();
        LogInfo << VAR(api_version) << VAR(filepath);

        if (api_version != kApiVersion) {
            LogError << "Unsupported API version" << VAR(api_version) << VAR(kApiVersion) << VAR(filepath);
            return false;
        }
    }
    else {
        LogError << "No API version function found" << VAR(filepath);
        return false;
    }

    Plugin plugin;
    if (library.has("OnPluginInit")) {
        plugin.on_init = library.get<decltype(OnPluginInit)>("OnPluginInit");
    }
    if (library.has("OnPluginFlush")) {
        plugin.on_flush = library.get<decltype(OnPluginFlush)>("OnPluginFlush");
    }
    if (library.has("OnPluginShutdown")) {
        plugin.on_shutdown = library.get<decltype(OnPluginShutdown)>("OnPluginShutdown");
    }

    constexpr const char* kFuncOnResourceEvent = "OnResourceEvent";
    if (library.has(kFuncOnResourceEvent)) {
        plugin.on_res_event = library.get<decltype(OnResourceEvent)>(kFuncOnResourceEvent);
        LogInfo << "Registered" << kFuncOnResourceEvent << VAR_VOIDP(plugin.on_res_event) << VAR(filepath);
    }
    else {
        LogWarn << "No func, ignore" << kFuncOnResourceEvent << VAR(filepath);
    }

    constexpr const char* kFuncOnControllerEvent = "OnControllerEvent";
    if (library.has(kFuncOnControllerEvent)) {
        plugin.on_ctrl_event = library.get<decltype(OnControllerEvent)>(kFuncOnControllerEvent);
        LogInfo << "Registered" << kFuncOnControllerEvent << VAR_VOIDP(plugin.on_ctrl_event) << VAR(filepath);
    }
    else {
        LogWarn << "No func, ignore" << kFuncOnControllerEvent << VAR(filepath);
    }

    constexpr const char* kFuncOnTaskerEvent = "OnTaskerEvent";
    if (library.has(kFuncOnTaskerEvent)) {
        plugin.on_tasker_event = library.get<decltype(OnTaskerEvent)>(kFuncOnTaskerEvent);
        LogInfo << "Registered" << kFuncOnTaskerEvent << VAR_VOIDP(plugin.on_tasker_event) << VAR(filepath);
    }
    else {
        LogWarn << "No func, ignore" << kFuncOnTaskerEvent << VAR(filepath);
    }

    constexpr const char* kFuncOnContextEvent = "OnContextEvent";
    if (library.has(kFuncOnContextEvent)) {
        plugin.on_ctx_event = library.get<decltype(OnContextEvent)>(kFuncOnContextEvent);
        LogInfo << "Registered" << kFuncOnContextEvent << VAR_VOIDP(plugin.on_ctx_event) << VAR(filepath);
    }
    else {
        LogWarn << "No func, ignore" << kFuncOnContextEvent << VAR(filepath);
    }

    plugin.library = std::move(library);

    plugins_.insert_or_assign(path_to_utf8_string(filepath), std::move(plugin));
    LogInfo << "Plugin registerd" << VAR(filepath);
    return true;
}

std::vector<std::string> PluginMgr::get_names() const
{
    std::lock_guard lock(mutex_);
    auto view = plugins_ | std::views::keys;
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_res_sinks() const
{
    std::lock_guard lock(mutex_);
    if (shutdown_) {
        return {};
    }
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_res_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_ctrl_sinks() const
{
    std::lock_guard lock(mutex_);
    if (shutdown_) {
        return {};
    }
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_ctrl_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_tasker_sinks() const
{
    std::lock_guard lock(mutex_);
    if (shutdown_) {
        return {};
    }
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_tasker_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_ctx_sinks() const
{
    std::lock_guard lock(mutex_);
    if (shutdown_) {
        return {};
    }
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_ctx_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

void PluginMgr::initialize(std::unique_lock<std::mutex>& lock)
{
    std::vector<void (*)()> callbacks;
    for (auto& plugin : plugins_ | std::views::values) {
        if (plugin.initialized) {
            continue;
        }
        plugin.initialized = true;
        if (plugin.on_init) {
            callbacks.push_back(plugin.on_init);
        }
    }
    managing_ = true;
    lock.unlock();
    for (auto callback : callbacks) {
        callback();
    }
    lock.lock();
    managing_ = false;
    management_done_.notify_all();
}

void PluginMgr::instance_created()
{
    std::unique_lock lock(mutex_);
    management_done_.wait(lock, [this] { return !managing_; });
    ++instances_;
    if (!shutdown_) {
        initialize(lock);
    }
}

void PluginMgr::instance_destroyed()
{
    std::lock_guard lock(mutex_);
    --instances_;
}

bool PluginMgr::flush()
{
    std::unique_lock lock(mutex_);
    if (shutdown_ || managing_) {
        return false;
    }
    initialize(lock);
    std::vector<void (*)()> callbacks;
    for (auto& plugin : plugins_ | std::views::values) {
        if (plugin.on_flush) {
            callbacks.push_back(plugin.on_flush);
        }
    }
    managing_ = true;
    lock.unlock();
    for (auto callback : callbacks) {
        callback();
    }
    lock.lock();
    managing_ = false;
    management_done_.notify_all();
    return true;
}

bool PluginMgr::shutdown()
{
    std::unique_lock lock(mutex_);
    if (managing_) {
        return false;
    }
    if (shutdown_) {
        return true;
    }
    if (instances_) {
        return false;
    }
    std::vector<void (*)()> callbacks;
    for (auto& plugin : plugins_ | std::views::values) {
        if (plugin.initialized && plugin.on_flush) {
            callbacks.push_back(plugin.on_flush);
        }
        if (plugin.initialized && plugin.on_shutdown) {
            callbacks.push_back(plugin.on_shutdown);
        }
    }
    shutdown_ = true;
    managing_ = true;
    lock.unlock();
    for (auto callback : callbacks) {
        callback();
    }
    lock.lock();
    managing_ = false;
    management_done_.notify_all();
    return true;
}

MAA_GLOBAL_NS_END
