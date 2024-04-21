#include <coroutine>
#include <format>
#include <iostream>
#include <memory>

#include "MaaPP/MaaPP.hpp"

maa::coro::Promise<void> async_main()
{
    auto res = std::make_shared<maa::Resource>();
    auto action = res->post_path(R"(E:\Projects\MAA\MaaAssistantSkland\assets\resource)");
    std::cout << "load posted, status " << action->status() << std::endl;
    auto status = co_await action->wait();
    std::cout << "load finish, status " << status << std::endl;

    auto devices = co_await maa::AdbDeviceFinder::find();

    for (const auto& dev : devices.value()) {
        std::cout
            << std::format("{}\n{}\n{}\n{}\n", dev.adb_path, dev.address, dev.type, dev.config)
            << std::endl;
    }

    auto ctrl = std::make_shared<maa::Controller>(
        maa::Controller::adb_controller_tag {},
        devices.value()[0],
        R"(E:\Projects\MAA\MaaFramework\3rdparty\MaaAgentBinary)");

    status = co_await ctrl->post_connect()->wait();
    std::cout << "connect finished, status " << status << std::endl;

    co_return;
}

int main()
{
    maa::coro::EventLoop ev;

    async_main().then([&ev]() { ev.defer_stop(); });

    ev.exec();
}
