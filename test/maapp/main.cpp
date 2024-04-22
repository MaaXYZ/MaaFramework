#include <chrono>
#include <coroutine>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

#include "MaaPP/MaaPP.hpp"
#include "MaaPP/coro/ThreadPool.hpp"
#include "MaaPP/maa/Instance.hpp"

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

            auto inst = std::make_shared<maa::Instance>();
            inst->bind(res);
            inst->bind(ctrl);
            std::cout << "instance inited " << inst->inited() << std::endl;
        }
    }
    co_return;
}

int main()
{
    maa::coro::EventLoop ev;

    async_main().then([&ev]() { ev.defer_stop(); });

    ev.exec();
}
