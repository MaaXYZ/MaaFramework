#include <coroutine>
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
    co_return;
}

int main()
{
    maa::coro::EventLoop ev;

    async_main().then([&ev]() { ev.defer_stop(); });

    ev.exec();
}
