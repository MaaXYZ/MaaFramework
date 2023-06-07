#include "MaaAPI.h"
#include "MaaCustomController.h"
#include "MaaParam.h"

#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

int main([[maybe_unused]] int argc, char** argv)
{
    const auto cur_path = std::filesystem::path(argv[0]).parent_path();

    MaaSetGlobalOption(MaaGlobalOption_Logging, (cur_path / "debug").string().c_str());

    auto res_handle = MaaResourceCreate((cur_path / "cache").string().c_str(), nullptr, nullptr);
    auto load_id = MaaResourcePostResource(res_handle, (cur_path / "resource").string().c_str());

    auto ctrl_handle = MaaAdbControllerCreate("adb.exe", "127.0.0.1:5555", nullptr, nullptr);
    auto connect_id = MaaControllerPostConnection(ctrl_handle);

    while (MaaResourceStatus(res_handle, load_id) == MaaStatus_Running) {
        std::this_thread::yield();
    }
    while (MaaControllerStatus(ctrl_handle, connect_id) == MaaStatus_Running) {
        std::this_thread::yield();
    }

    auto inst = MaaCreate(nullptr, nullptr);

    MaaBindResource(inst, res_handle);
    MaaBindController(inst, ctrl_handle);

    auto task_id = MaaPostTask(inst, "", "{}");

    while (!MaaTaskAllFinished(inst)) {
        std::this_thread::yield();
    }

    MaaDestroy(&inst);
    MaaResourceDestroy(&res_handle);
    MaaControllerDestroy(&ctrl_handle);

    return 0;
}
