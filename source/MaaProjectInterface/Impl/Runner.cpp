#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaPP/MaaPP.hpp"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "Utils/Logger.h"
#include "Utils/ScopeLeave.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Runner::run(
    const RuntimeParam& param,
    MaaInstanceCallback callback,
    MaaCallbackTransparentArg callback_arg,
    MaaResourceCallback resource_callback,
    MaaCallbackTransparentArg resource_callback_arg,
    MaaControllerCallback controller_callback,
    MaaCallbackTransparentArg controller_callback_arg)
{
    auto run = [&]() -> maa::coro::Promise<bool> {
        auto maa_handle = maa::Instance::make([&](auto msg, auto detail) {
            return callback(msg.data(), detail.to_string().c_str(), callback_arg);
        });

        std::shared_ptr<maa::Controller> controller_handle;
        if (const auto* p_adb_param =
                std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
            controller_handle = maa::Controller::make(
                maa::Controller::adb_controller_tag {},
                p_adb_param->adb_path,
                p_adb_param->address,
                p_adb_param->controller_type,
                p_adb_param->config,
                p_adb_param->agent_path);
        }
        else if (
            const auto* p_win32_param =
                std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
            controller_handle = maa::Controller::make(
                maa::Controller::win32_controller_tag {},
                p_win32_param->hwnd,
                p_win32_param->controller_type);
        }
        else {
            LogError << "Unknown controller type";
            co_return false;
        }

        auto resource_handle = maa::Resource::make();

        auto do_conn = [&]() -> maa::coro::Promise<bool> {
            co_return co_await controller_handle->post_connect()->wait() == MaaStatus_Failed;
        };

        auto do_res = [&]() -> maa::coro::Promise<bool> {
            for (const auto& path : param.resource_path) {
                if (co_await resource_handle->post_path(path)->wait() == MaaStatus_Failed) {
                    co_return false;
                }
            }
            co_return true;
        };

        auto [ctrl_success, res_success] = co_await maa::coro::all(do_conn(), do_res());

        if (!ctrl_success) {
            LogError << "Failed to connect controller";
            co_return false;
        }

        if (!res_success) {
            LogError << "Failed to load resource";
            co_return false;
        }

        co_return true;
    };

    auto maa_handle = MaaCreate(callback, callback_arg);

    MaaControllerHandle controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        controller_handle = MaaAdbControllerCreateV2(
            p_adb_param->adb_path.c_str(),
            p_adb_param->address.c_str(),
            p_adb_param->controller_type,
            p_adb_param->config.c_str(),
            p_adb_param->agent_path.c_str(),
            controller_callback,
            controller_callback_arg);
    }
    else if (
        const auto* p_win32_param =
            std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle = MaaWin32ControllerCreate(
            p_win32_param->hwnd,
            p_win32_param->controller_type,
            controller_callback,
            controller_callback_arg);
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    auto resource_handle = MaaResourceCreate(resource_callback, resource_callback_arg);

    int64_t cid = MaaControllerPostConnection(controller_handle);
    int64_t rid = 0;
    for (const auto& path : param.resource_path) {
        rid = MaaResourcePostPath(resource_handle, path.c_str());
    }

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    if (MaaStatus_Failed == MaaControllerWait(controller_handle, cid)) {
        LogError << "Failed to connect controller";
        return false;
    }

    if (MaaStatus_Failed == MaaResourceWait(resource_handle, rid)) {
        LogError << "Failed to load resource";
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
