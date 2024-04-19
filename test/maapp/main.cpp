#include <coroutine>
#include <iostream>
#include <memory>

#include "MaaPP/MaaPP.hpp"

struct GlobalChunk
{
    std::shared_ptr<maa::Resource> res;
};

maa::utils::Task<void> test()
{
    co_return;
}

maa::utils::Task<void> async_main(maa::utils::EventLoop& loop, GlobalChunk& chunk)
{
    co_await test();
    // chunk.res = std::make_shared<maa::Resource>();

    // std::cout << "resource created" << std::endl;

    // auto action = chunk.res->post_path(
    //     "/Users/nekosu/Documents/Projects/MAA/MaaAssistantSkland/assets/resource");

    // std::cout << "got action" << std::endl;

    // auto status = co_await action->wait();

    // std::cout << status << std::endl;

    // loop.stop();
}

int main()
{
    auto val = MaaLoggingLevel_Error;
    MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &val, sizeof(val));

    maa::utils::EventLoop loop;

    loop.bind();

    GlobalChunk chunk;
    auto h = async_main(loop, chunk);

    std::cout << "enter loop" << std::endl;

    loop.exec();

    return 0;
}
