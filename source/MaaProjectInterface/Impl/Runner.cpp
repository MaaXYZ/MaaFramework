#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "Utils/ScopeLeave.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Runner::run(
    const MAA_PROJECT_INTERFACE_NS::RuntimeParam& param, //
    MaaInstanceCallback callback,
    MaaCallbackTransparentArg callback_arg,
    MaaResourceCallback resource_callback,
    MaaCallbackTransparentArg resource_callback_arg,
    MaaControllerCallback controller_callback,
    MaaCallbackTransparentArg controller_callback_arg)
{
    auto maa_handle = MaaCreate(callback, callback_arg);

    auto controller_handle = MaaAdbControllerCreateV2(
        param.adb_param.adb_path.c_str(),
        param.adb_param.address.c_str(),
        param.adb_param.controller_type,
        param.adb_param.config.c_str(),
        param.adb_param.agent_path.c_str(),
        controller_callback,
        controller_callback_arg);
    auto resource_handle = MaaResourceCreate(resource_callback, resource_callback_arg);

    int64_t cid = MaaControllerPostConnection(controller_handle);
    int64_t rid = 0;
    for (const auto& path : param.resource_path) {
        rid = MaaResourcePostPath(resource_handle, path.c_str());
    }

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    if (MaaStatus_Failed == MaaControllerWait(controller_handle, cid)) {
        std::cout << "Failed to connect to device:\n"
                  << param.adb_param.adb_path << "\n"
                  << param.adb_param.address << std::endl;
        return false;
    }

    if (MaaStatus_Failed == MaaResourceWait(resource_handle, rid)) {
        std::cout << "Failed to load resource" << std::endl;
        return false;
    }

    OnScopeLeave([&]() {
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    });

    for (const auto& [name, executor] : param.recognizer) {
        std::string exec_param = json::array(executor.exec_param).to_string();
        MaaToolkitRegisterCustomRecognizerExecutor(
            maa_handle,
            name.c_str(),
            executor.exec_path.c_str(),
            exec_param.c_str());
    }
    for (const auto& [name, executor] : param.action) {
        std::string exec_param = json::array(executor.exec_param).to_string();
        MaaToolkitRegisterCustomActionExecutor(
            maa_handle,
            name.c_str(),
            executor.exec_path.c_str(),
            exec_param.c_str());
    }

    int64_t tid = 0;
    for (const auto& task : param.task) {
        std::string task_param = task.param.to_string();
        tid = MaaPostTask(maa_handle, task.entry.c_str(), task_param.c_str());
    }

    MaaWaitTask(maa_handle, tid);

    return true;
}

MAA_PROJECT_INTERFACE_NS_END