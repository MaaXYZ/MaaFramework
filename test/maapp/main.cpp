#include <chrono>
#include <coroutine>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

#include "MaaPP/MaaPP.hpp"

#ifdef _WIN32
std::string res_path = R"(E:\Projects\MAA\MaaAssistantSkland\assets\resource)";
std::string agent_path = R"(E:\Projects\MAA\MaaFramework\3rdparty\MaaAgentBinary)";
#else
std::string res_path = R"(/Users/nekosu/Documents/Projects/MAA/MaaAssistantSkland/assets/resource)";
std::string agent_path =
    R"(/Users/nekosu/Documents/Projects/MAA/MaaFramework/3rdparty/MaaAgentBinary)";
#endif

maa::coro::Promise<void> async_main()
{
    auto res = maa::Resource::make();
    auto action = res->post_path(res_path);
    std::cout << "load posted, status " << action->status() << std::endl;
    auto status = co_await action->wait();
    std::cout << "load finish, status " << status << std::endl;

    auto devices = co_await maa::AdbDeviceFinder::find();

    if (devices) {
        for (const auto& dev : *devices) {
            std::cout << std::format(
                "{}\n{}\n{}\n{}\n",
                dev.adb_path,
                dev.address,
                dev.type.type_,
                dev.config)
                      << std::endl;
        }

        if (devices->size() > 0) {
            auto cfg = (*devices)[0];
            cfg.type.set_screencap(MaaAdbControllerType_Screencap_Encode);
            auto ctrl =
                maa::Controller::make(maa::Controller::adb_controller_tag {}, cfg, agent_path);

            status = co_await ctrl->post_connect()->wait();
            std::cout << "connect finished, status " << status << std::endl;
            std::cout << "controller uuid: " << ctrl->uuid().value() << std::endl;

            auto inst = maa::Instance::make();
            inst->bind(res);
            inst->bind(ctrl);
            std::cout << "instance inited " << inst->inited() << std::endl;

            inst->bind(
                "TestAct",
                std::make_shared<maa::CustomAction>(
                    []([[maybe_unused]] auto sync_context,
                       [[maybe_unused]] MaaStringView task_name,
                       [[maybe_unused]] MaaStringView custom_action_param,
                       [[maybe_unused]] const MaaRect& cur_box,
                       [[maybe_unused]] MaaStringView cur_rec_detail) -> maa::coro::Promise<bool> {
                        co_await maa::coro::EventLoop::current()->sleep(std::chrono::seconds(5));
                        std::cout << "Hello world!" << std::endl;
                        co_return true;
                    }));

            // maa::set_stdout_level(MaaLoggingLevel_All);

            auto task = inst->post_task("test");
            std::cout << "post task" << std::endl;
            auto success = co_await maa::coro::Promise<void>::any(
                { task->wait().then([](auto) {}),
                  maa::coro::EventLoop::current()->sleep(std::chrono::seconds(2)) });
            if (success == 1) {
                std::cout << "post stop" << std::endl;
                inst->stop();
                co_await task->wait();
            }
        }
    }
    co_return;
}

int main()
{
    maa::set_stdout_level(MaaLoggingLevel_Fatal);

    maa::coro::EventLoop ev;

    async_main().then([&ev]() { ev.defer_stop(); });

    ev.exec();
}
