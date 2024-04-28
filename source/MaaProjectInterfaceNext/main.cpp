#include "MaaPP/MaaPP.hpp"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

maa::coro::Promise<int> async_main()
{
    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    maa::set_stdout_level(MaaLoggingLevel_Error);

    maa::init(user_path);

    co_return 0;
}

int main()
{
    maa::coro::EventLoop ev;

    ev.stop_after(async_main());

    return ev.exec();
}
