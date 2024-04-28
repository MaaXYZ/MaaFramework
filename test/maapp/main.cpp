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

struct MyProxy : public maa::details::ProxyController
{
    using ProxyController::ProxyController;

    virtual maa::coro::Promise<bool> connect() override
    {
        std::cout << "connect called!" << std::endl;
        co_return co_await ProxyController::connect();
    }
};

maa::coro::Promise<> async_main()
{
    // #if !defined(__GNUC__) || defined(__clang__)
    // 目前gcc有bug, 无法编译下面这个代码, 会触发ICE
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
            auto ctrl = maa::Controller::make(cfg, agent_path);

            ctrl->set_short_side(720);
            ctrl->set_start_entry("com.hypergryph.skland/com.hypergryph.skland.SplashActivity");
            ctrl->set_stop_entry("com.hypergryph.skland");

            auto proxy_handler = std::make_shared<MyProxy>(ctrl);
            proxy_handler->width_ = 720;
            proxy_handler->height_ = 1080;
            auto proxy_ctrl = maa::Controller::make(proxy_handler);

            status = co_await proxy_ctrl->post_connect()->wait();
            std::cout << "connect finished, status " << status << std::endl;
            std::cout << "controller uuid: " << proxy_ctrl->uuid().value() << std::endl;

            auto inst = maa::Instance::make();
            inst->bind(res);
            inst->bind(proxy_ctrl);
            std::cout << "instance inited " << inst->inited() << std::endl;

            inst->bind(
                "TestAct",
                maa::CustomAction::make(
                    []([[maybe_unused]] auto sync_context,
                       [[maybe_unused]] MaaStringView task_name,
                       [[maybe_unused]] MaaStringView custom_action_param,
                       [[maybe_unused]] const MaaRect& cur_box,
                       [[maybe_unused]] MaaStringView cur_rec_detail) -> maa::coro::Promise<bool> {
                        co_await maa::coro::EventLoop::current()->sleep(std::chrono::seconds(5));
                        std::cout << "Hello world!" << std::endl;
                        co_return true;
                    }));
            maa::set_stdout_level(MaaLoggingLevel_All);

            auto task = inst->post_task("Skland");
            std::cout << "post task" << std::endl;
            auto success = co_await maa::coro::any(
                task->wait().then([](auto) { std::cout << "task finished" << std::endl; }),
                maa::coro::EventLoop::current()->sleep(std::chrono::seconds(30)).then([]() {
                    std::cout << "sleep finished" << std::endl;
                }));
            std::cout << "promise any finished, index " << success.index() << std::endl;
            if (success.index() == 1) {
                std::cout << "post stop" << std::endl;
                inst->stop();
                co_await task->wait();
            }
        }
    }
    // #endif
    co_return;
}

int main()
{
    maa::set_stdout_level(MaaLoggingLevel_Fatal);

    maa::coro::EventLoop ev;

    ev.stop_after(async_main());

    ev.exec();
}
