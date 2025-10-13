#include "MaaToolkit/PluginSystem/MaaToolkitPluginSystem.h"

#include "PluginSystem/PluginMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaBool MaaToolkitPluginSystemLoadLibrary(const char* library_name, MaaTasker* tasker, MaaResource* resource, MaaController* controller)
{
    LogFunc << VAR(library_name) << VAR_VOIDP(tasker) << VAR_VOIDP(resource) << VAR_VOIDP(controller);

    return MAA_TOOLKIT_NS::PluginMgr::get_instance().load_library(MAA_NS::path(library_name), tasker, resource, controller);
}

MaaBool MaaToolkitPluginSystemLoadDirectory(const char* directory_path, MaaTasker* tasker, MaaResource* resource, MaaController* controller)
{
    LogFunc << VAR(directory_path) << VAR_VOIDP(tasker) << VAR_VOIDP(resource) << VAR_VOIDP(controller);

    return MAA_TOOLKIT_NS::PluginMgr::get_instance().load_directory(MAA_NS::path(directory_path), tasker, resource, controller);
}
