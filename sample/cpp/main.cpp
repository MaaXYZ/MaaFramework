#include "MaaAPI.h"

#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

bool demo_polling(const std::filesystem::path& cur_dir);
bool demo_waiting(const std::filesystem::path& cur_dir);

#ifdef _WIN32
const char* adb = "adb.exe";
#else
const char* adb = "adb";
#endif

std::string adbAddress;

int main([[maybe_unused]] int argc, char** argv)
{
    if (argc > 1) {
        adbAddress = argv[1];
    }
    else {
        adbAddress = "127.0.0.1:5555";
    }

    const auto cur_dir = std::filesystem::path(argv[0]).parent_path();

    MaaSetGlobalOption(MaaGlobalOption_Logging, (cur_dir / "debug").string().c_str());

    demo_polling(cur_dir);
    demo_waiting(cur_dir);

    return 0;
}

bool demo_polling(const std::filesystem::path& cur_dir)
{
    auto resource_handle = MaaResourceCreate((cur_dir / "cache").string().c_str(), nullptr, nullptr);
    auto resource_id = MaaResourcePostResource(resource_handle, (cur_dir / "resource").string().c_str());

    auto controller_handle = MaaAdbControllerCreate(adb, adbAddress.c_str(), nullptr, nullptr);
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

    auto destroy = [&]() {
        MaaDestroy(&maa_handle);
        MaaResourceDestroy(&resource_handle);
        MaaControllerDestroy(&controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        destroy();
        return false;
    }

    MaaPostTask(maa_handle, MaaTaskType_Pipeline, MaaTaskParam_Empty);

    while (!MaaTaskAllFinished(maa_handle)) {
        std::this_thread::yield();
    }

    destroy();

    return true;
}

bool demo_waiting(const std::filesystem::path& cur_dir)
{
    auto maa_handle = MaaCreate(nullptr, nullptr);
    auto resource_handle = MaaResourceCreate((cur_dir / "cache").string().c_str(), nullptr, nullptr);
    auto controller_handle = MaaAdbControllerCreate(adb, adbAddress.c_str(), nullptr, nullptr);

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    auto resource_id = MaaResourcePostResource(resource_handle, (cur_dir / "resource").string().c_str());
    auto connection_id = MaaControllerPostConnection(controller_handle);

    MaaResourceWait(resource_handle, resource_id);
    MaaControllerWait(controller_handle, connection_id);

    auto destroy = [&]() {
        MaaDestroy(&maa_handle);
        MaaResourceDestroy(&resource_handle);
        MaaControllerDestroy(&controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        destroy();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, MaaTaskType_Pipeline, MaaTaskParam_Empty);
    MaaTaskWait(maa_handle, task_id);

    destroy();

    return true;
}
