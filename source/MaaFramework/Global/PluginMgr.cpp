#include "PluginMgr.h"

#include <ranges>

#include "MaaFramework/MaaAPI.h"
#include "MaaPlugin/MaaPluginAPI.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

MAA_GLOBAL_NS_BEGIN

struct DefaultPluginsLoader
{
    DefaultPluginsLoader() { PluginMgr::get_instance().load(library_dir() / "plugins"); }
} _;

bool PluginMgr::load(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

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

    constexpr uint32_t kApiVersion = 1;
    constexpr const char* kFuncGetApiVersion = "GetApiVersion";
    if (library.has(kFuncGetApiVersion)) {
        auto func = library.get<decltype(GetApiVersion)>(kFuncGetApiVersion);
        if (!func) {
            LogError << "Failed to get function" << VAR(library_path) << VAR(kFuncGetApiVersion);
            return false;
        }
        uint32_t version = func();
        if (version != kApiVersion) {
            LogError << "Unsupported API version" << VAR(library_path) << VAR(version) << VAR(kApiVersion);
            return false;
        }
    }
    else {
        LogError << "No API version function found" << VAR(library_path);
        return false;
    }

    Plugin plugin;

    constexpr const char* kFuncOnResourceEvent = "OnResourceEvent";
    if (library.has(kFuncOnResourceEvent)) {
        plugin.on_res_event = library.get<decltype(OnResourceEvent)>(kFuncOnResourceEvent);
    }

    constexpr const char* kFuncOnControllerEvent = "OnControllerEvent";
    if (library.has(kFuncOnControllerEvent)) {
        plugin.on_ctrl_event = library.get<decltype(OnControllerEvent)>(kFuncOnControllerEvent);
    }

    constexpr const char* kFuncOnTaskerEvent = "OnTaskerEvent";
    if (library.has(kFuncOnTaskerEvent)) {
        plugin.on_tasker_event = library.get<decltype(OnTaskerEvent)>(kFuncOnTaskerEvent);
    }

    constexpr const char* kFuncOnContextEvent = "OnContextEvent";
    if (library.has(kFuncOnContextEvent)) {
        plugin.on_ctrl_event = library.get<decltype(OnContextEvent)>(kFuncOnContextEvent);
    }

    std::filesystem::path filepath = library.location();
    plugin.library = std::move(library);

    plugins_.insert_or_assign(path_to_utf8_string(filepath), std::move(plugin));
    return true;
}

std::vector<std::string> PluginMgr::get_names() const
{
    auto view = plugins_ | std::views::keys;
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_res_sinks() const
{
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_res_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_ctrl_sinks() const
{
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_ctrl_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_tasker_sinks() const
{
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_tasker_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

std::vector<PluginMgr::Sink> PluginMgr::get_ctx_sinks() const
{
    auto view = plugins_ | std::views::values | std::views::transform(&Plugin::on_ctx_event)
                | std::views::filter([](const Sink& s) { return s != nullptr; });
    return { view.begin(), view.end() };
}

MAA_GLOBAL_NS_END
