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
        auto maa_handle =
            maa::Instance::make([&](auto msg) { msg->forward(callback, callback_arg); });

        maa::ControllerHandle controller_handle;
        if (const auto* p_adb_param =
                std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
            controller_handle = maa::Controller::make(
                maa::Controller::adb_controller_tag {},
                p_adb_param->adb_path,
                p_adb_param->address,
                p_adb_param->controller_type,
                p_adb_param->config,
                p_adb_param->agent_path,
                [&](auto msg) { msg->forward(controller_callback, controller_callback_arg); });
        }
        else if (
            const auto* p_win32_param =
                std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
            controller_handle = maa::Controller::make(
                maa::Controller::win32_controller_tag {},
                p_win32_param->hwnd,
                p_win32_param->controller_type,
                [&](auto msg) { msg->forward(controller_callback, controller_callback_arg); });
        }
        else {
            LogError << "Unknown controller type";
            co_return false;
        }

        auto resource_handle = maa::Resource::make(
            [&](auto msg) { msg->forward(resource_callback, resource_callback_arg); });

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

        for (const auto& [name, executor] : param.recognizer) {
            maa_handle->bind_recognizer_executor(name, executor.exec_path, executor.exec_param);
        }
        for (const auto& [name, executor] : param.action) {
            maa_handle->bind_action_executor(name, executor.exec_path, executor.exec_param);
        }

        std::vector<maa::coro::Promise<MaaStatus>> task_results;

        for (const auto& task : param.task) {
            std::string task_param = task.param.to_string();
            task_results.push_back(maa_handle->post_task(task.entry, task.param)->wait());
        }

        co_await maa::coro::all(task_results);

        co_return true;
    };

    return run().sync_wait();
}

MAA_PROJECT_INTERFACE_NS_END
