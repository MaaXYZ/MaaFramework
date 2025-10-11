#include "PluginMgr.h"

#include "MaaFramework/MaaAPI.h"
#include "MaaPlugin/MaaPluginAPI.h"

#include "Utils/Logger.h"
#include "Utils/Runtime.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

bool PluginMgr::load_library(const std::filesystem::path& library_name, MaaTasker* tasker, MaaResource* resource, MaaController* controller)
{
    LogFunc << VAR(library_name) << VAR_VOIDP(tasker) << VAR_VOIDP(resource) << VAR_VOIDP(controller);

    return load_and_register(library_dir() / library_name, tasker, resource, controller)
           || load_and_register(library_name, tasker, resource, controller);
}

bool PluginMgr::load_directory(
    const std::filesystem::path& directory_path,
    MaaTasker* tasker,
    MaaResource* resource,
    MaaController* controller)
{
    LogFunc << VAR(directory_path) << VAR_VOIDP(tasker) << VAR_VOIDP(resource) << VAR_VOIDP(controller);

    bool ret = false;
    for (auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
        ret |= load_and_register(std::filesystem::absolute(entry.path()), tasker, resource, controller);
    }

    return ret;
}

std::optional<boost::dll::shared_library> PluginMgr::load_dll(const std::filesystem::path& library_path)
{
    LogFunc << VAR(library_path);

    if (auto it = libraries_.find(path_to_utf8_string(library_path)); it != libraries_.end()) {
        LogDebug << "Library already loaded" << VAR(library_path);
        return it->second;
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

bool PluginMgr::load_and_register(
    const std::filesystem::path& library_path,
    MaaTasker* tasker,
    MaaResource* resource,
    MaaController* controller)
{
    LogFunc << VAR(library_path) << VAR_VOIDP(tasker) << VAR_VOIDP(resource) << VAR_VOIDP(controller);

    auto opt = load_dll(library_path);
    if (!opt) {
        LogError << "Failed to load DLL" << VAR(library_path);
        return false;
    }

    auto& library = *opt;

    MaaBool loaded = false;

    constexpr const char* kFuncOnResourceEvent = "OnResourceEvent";
    if (library.has(kFuncOnResourceEvent)) {
        auto func = library.get<decltype(OnResourceEvent)>(kFuncOnResourceEvent);
        if (!func) {
            LogError << "Failed to get function" << VAR(library_path) << VAR(kFuncOnResourceEvent);
            return false;
        }
        loaded |= MaaResourceAddSink(resource, func, nullptr);
    }

    constexpr const char* kFuncOnControllerEvent = "OnControllerEvent";
    if (library.has(kFuncOnControllerEvent)) {
        auto func = library.get<decltype(OnControllerEvent)>(kFuncOnControllerEvent);
        if (!func) {
            LogError << "Failed to get function" << VAR(library_path) << VAR(kFuncOnControllerEvent);
            return false;
        }
        loaded |= MaaControllerAddSink(controller, func, nullptr);
    }

    constexpr const char* kFuncOnTaskerEvent = "OnTaskerEvent";
    if (library.has(kFuncOnTaskerEvent)) {
        auto func = library.get<decltype(OnTaskerEvent)>(kFuncOnTaskerEvent);
        if (!func) {
            LogError << "Failed to get function" << VAR(library_path) << VAR(kFuncOnTaskerEvent);
            return false;
        }

        loaded |= MaaTaskerAddSink(tasker, func, nullptr);
    }

    constexpr const char* kFuncOnContextEvent = "OnContextEvent";
    if (library.has(kFuncOnContextEvent)) {
        auto func = library.get<decltype(OnContextEvent)>(kFuncOnContextEvent);
        if (!func) {
            LogError << "Failed to get function" << VAR(library_path) << VAR(kFuncOnContextEvent);
            return false;
        }

        loaded |= MaaTaskerAddContextSink(tasker, func, nullptr);
    }

    if (loaded) {
        auto filepath = library.location();
        libraries_.insert_or_assign(path_to_utf8_string(filepath), std::move(library));
    }
    else {
        LogError << "No valid plugin interface found" << VAR(library_path);
    }

    return loaded;
}

MAA_TOOLKIT_NS_END
