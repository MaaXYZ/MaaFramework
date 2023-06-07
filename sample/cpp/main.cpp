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

    auto resource_handle = MaaResourceCreate((cur_path / "cache").string().c_str(), nullptr, nullptr);
    auto resource_id = MaaResourcePostResource(resource_handle, (cur_path / "resource").string().c_str());

    auto controller_handle = MaaAdbControllerCreate("adb.exe", "127.0.0.1:5555", nullptr, nullptr);
    auto connection_id = MaaControllerPostConnection(controller_handle);

    for (auto status = MaaResourceStatus(resource_handle, resource_id);
         status == MaaStatus_Pending || status == MaaStatus_Running;
         status = MaaResourceStatus(resource_handle, resource_id)) {
        std::this_thread::yield();
    }
    for (auto status = MaaControllerStatus(controller_handle, connection_id);
         status == MaaStatus_Pending || status == MaaStatus_Running;
         status = MaaControllerStatus(controller_handle, connection_id)) {
        std::this_thread::yield();
    }

    auto maa_handle = MaaCreate(nullptr, nullptr);

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    auto task_id = MaaPostTask(maa_handle, MaaTaskType_Pipeline, MaaTaskParam_Empty);

    while (!MaaTaskAllFinished(maa_handle)) {
        std::this_thread::yield();
    }

    MaaDestroy(&maa_handle);
    MaaResourceDestroy(&resource_handle);
    MaaControllerDestroy(&controller_handle);

    return 0;
}
