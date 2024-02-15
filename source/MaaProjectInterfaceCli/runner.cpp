#include "runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "Utils/Format.hpp"
#include "Utils/ScopeLeave.hpp"

bool Runner::run(const MAA_PROJECT_INTERFACE_NS::RuntimeParam& param)
{
    MaaToolkitInit();

    auto maa_handle = MaaCreate(&Runner::on_maafw_notify, nullptr);

    auto controller_handle = MaaAdbControllerCreateV2(param.adb_param.adb_path.c_str(), param.adb_param.address.c_str(),
                                                      param.adb_param.controller_type, param.adb_param.config.c_str(),
                                                      param.adb_param.agent_path.c_str(), nullptr, nullptr);
    auto resource_handle = MaaResourceCreate(nullptr, nullptr);

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
        MaaToolkitUninit();
    });

    for (const auto& [name, executor] : param.recognizer) {
        std::string exec_param = json::array(executor.exec_param).to_string();
        MaaToolkitRegisterCustomRecognizerExecutor(maa_handle, name.c_str(), executor.exec_path.c_str(),
                                                   exec_param.c_str());
    }
    for (const auto& [name, executor] : param.action) {
        std::string exec_param = json::array(executor.exec_param).to_string();
        MaaToolkitRegisterCustomActionExecutor(maa_handle, name.c_str(), executor.exec_path.c_str(),
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

void Runner::on_maafw_notify(MaaStringView msg, MaaStringView details_json, MaaTransparentArg callback_arg)
{
    std::ignore = callback_arg;

    std::string entry = json::parse(details_json).value_or(json::value())["entry"].as_string();
    std::cout << MAA_FMT::format("on_maafw_notify: {} {}", msg, entry) << std::endl;
}
