#include "LibraryHolder/AgentClient.h"

#include "MaaAgentClient/MaaAgentClientAPI.h"

#include <filesystem>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Runtime.h"

MAA_NS_BEGIN

std::shared_ptr<MaaAgentClient> AgentClientLibraryHolder::create_agent_client()
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    auto create_func = get_function<decltype(MaaAgentClientCreateV2)>(create_func_name_);
    if (!create_func) {
        LogError << "Failed to get function create_func" << VAR(create_func_name_);
        return nullptr;
    }

    auto destroy_func = get_function<decltype(MaaAgentClientDestroy)>(destroy_func_name_);
    if (!destroy_func) {
        LogError << "Failed to get function destroy_func" << VAR(destroy_func_name_);
        return nullptr;
    }

    auto handle = create_func(nullptr);
    if (!handle) {
        LogError << "Failed to create handle";
        return nullptr;
    }

    return std::shared_ptr<MaaAgentClient>(handle, destroy_func);
}

MAA_NS_END
