#include "MaaAPI.h"

#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

int main([[maybe_unused]] int argc, char** argv)
{
    const auto cur_path = std::filesystem::path(argv[0]).parent_path();

    auto res = MaaResourceCreate((cur_path / "resource").string().c_str(), (cur_path / "cache").string().c_str(),
                                 nullptr, nullptr);
    auto ctrl = MaaAdbControllerCreate("adb.exe", "127.0.0.1:5555", "{}", nullptr, nullptr);
    auto inst = MaaCreate(nullptr, nullptr);

    MaaBindResource(inst, res);
    MaaBindController(inst, ctrl);

    MaaPostTask(inst, "", "{}");

    while (MaaRunning(inst)) {
        std::this_thread::yield();
    }
    MaaStop(inst);

    MaaDestroy(&inst);
    MaaResourceDestroy(&res);
    MaaControllerDestroy(&ctrl);

    return 0;
}
