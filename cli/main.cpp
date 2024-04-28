#include <MaaPP/MaaPP.hpp>

#include "UI/Menu.h"
#include "Utils/Misc.h"

using namespace maa;

coro::Promise<int> async_main()
{
    set_stdout_level(MaaLoggingLevel_Error);

    init(cli::program_directory());

    co_return 0;
}

int main()
{
    coro::EventLoop ev;

    ev.stop_after(async_main());

    return ev.exec();
}
