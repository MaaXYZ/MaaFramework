#include <chrono>
#include <coroutine>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

#include "MaaPP/MaaPP.hpp"
#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"

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
    auto res = std::make_shared<maa::Resource>();
    auto action = res->post_path(res_path);
    std::cout << "load posted, status " << action->status() << std::endl;
    auto status = co_await action->wait();
    std::cout << "load finish, status " << status << std::endl;

    auto devices = co_await maa::AdbDeviceFinder::find();

    if (devices) {
        for (const auto& dev : *devices) {
            std::cout
                << std::format("{}\n{}\n{}\n{}\n", dev.adb_path, dev.address, dev.type, dev.config)
                << std::endl;
        }

        if (devices->size() > 0) {
            auto ctrl = std::make_shared<maa::Controller>(
                maa::Controller::adb_controller_tag {},
                (*devices)[0],
                agent_path);

            status = co_await ctrl->post_connect()->wait();
            std::cout << "connect finished, status " << status << std::endl;
            std::cout << "controller uuid: " << ctrl->uuid().value() << std::endl;

            auto inst = std::make_shared<maa::Instance>();
            inst->bind(res);
            inst->bind(ctrl);
            std::cout << "instance inited " << inst->inited() << std::endl;

            auto custom_action = []([[maybe_unused]] MaaSyncContextHandle sync_context,
                                    [[maybe_unused]] MaaStringView task_name,
                                    [[maybe_unused]] MaaStringView custom_action_param,
                                    [[maybe_unused]] const MaaRect& cur_box,
                                    [[maybe_unused]] MaaStringView cur_rec_detail,
                                    maa::coro::Promise<void> stop) -> maa::coro::Promise<bool> {
                auto task = []() -> maa::coro::Promise<void> {
                    co_await maa::coro::EventLoop::current()->sleep(std::chrono::seconds(5));
                    std::cout << "Hello world!" << std::endl;
                    co_return;
                };
                std::initializer_list<Promise<void>> list = { task(), stop };
                auto success = co_await maa::coro::Promise<void>::any(list);
                if (success == 1) {
                    std::cout << "Task cancelled!" << std::endl;
                    co_return false;
                }
                else {
                    co_return true;
                }
            };

            inst->bind("TestAct", std::make_shared<maa::CustomAction>(custom_action));

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
