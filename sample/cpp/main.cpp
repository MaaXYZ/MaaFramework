#include "MaaFramework/MaaAPI.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

bool demo_waiting();
bool demo_polling();
std::string read_adb_config(const std::filesystem::path& cur_dir);

std::string adb = "adb";
std::string adb_address = "127.0.0.1:5555";
std::string adb_config;

std::string debug_dir;
std::string resource_dir;

std::string task_name = "StartUp";

int main([[maybe_unused]] int argc, char** argv)
{
    const auto cur_dir = std::filesystem::path(argv[0]).parent_path();

    debug_dir = (cur_dir / "debug").string();
    resource_dir = (cur_dir / "resource").string();

    if (argc >= 3) {
        adb = argv[1];
        adb_address = argv[2];
    }
    if (argc >= 4) {
        resource_dir = argv[3];
    }
    if (argc >= 5) {
        task_name = argv[4];
    }

    adb_config = read_adb_config(cur_dir);

    MaaSetGlobalOption(MaaGlobalOption_Logging, (void*)debug_dir.c_str(), debug_dir.size());

    // demo_polling();
    demo_waiting();

    return 0;
}

bool demo_waiting()
{
    auto maa_handle = MaaCreate(nullptr, nullptr);
    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto controller_handle =
        MaaAdbControllerCreate(adb.c_str(), adb_address.c_str(),
                               MaaAdbControllerType_Input_Preset_Adb | MaaAdbControllerType_Screencap_RawWithGzip,
                               adb_config.c_str(), nullptr, nullptr);

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);
    int height = 720;
    MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetHeight, reinterpret_cast<void*>(&height),
                           sizeof(int));

    auto resource_id = MaaResourcePostResource(resource_handle, resource_dir.c_str());
    auto connection_id = MaaControllerPostConnection(controller_handle);

    MaaResourceWait(resource_handle, resource_id);
    MaaControllerWait(controller_handle, connection_id);

    auto destroy = [&]() {
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        destroy();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, task_name.c_str(), MaaTaskParam_Empty);
    MaaTaskWait(maa_handle, task_id);

    destroy();

    return true;
}

bool demo_polling()
{
    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto resource_id = MaaResourcePostResource(resource_handle, resource_dir.c_str());

    auto controller_handle =
        MaaAdbControllerCreate(adb.c_str(), adb_address.c_str(),
                               MaaAdbControllerType_Input_Preset_Minitouch | MaaAdbControllerType_Screencap_FastestWay,
                               adb_config.c_str(), nullptr, nullptr);
    auto connection_id = MaaControllerPostConnection(controller_handle);
    int height = 720;
    MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetHeight, reinterpret_cast<void*>(&height),
                           sizeof(int));
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
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        destroy();
        return false;
    }

    MaaPostTask(maa_handle, task_name.c_str(), MaaTaskParam_Empty);

    while (!MaaTaskAllFinished(maa_handle)) {
        std::this_thread::yield();
    }

    destroy();

    return true;
}

std::string read_adb_config(const std::filesystem::path& cur_dir)
{
    std::ifstream ifs(cur_dir / "controller_config.json", std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open controller_config.json\n"
                  << "Please copy sample/cpp/config/controller_config.json to " << cur_dir << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}
