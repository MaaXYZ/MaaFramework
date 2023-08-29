#include "MaaFramework/MaaAPI.h"
#include "MaaToolKit/MaaToolKitAPI.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int main([[maybe_unused]] int argc, char** argv)
{
    MaaToolKitInit();
    auto device_size = MaaToolKitFindDevice();
    if (device_size == 0) {
        std::cout << "No device found" << std::endl;
        return -1;
    }

    const int kIndex = 0; // for demo, we just use the first device
    auto controller_handle = MaaAdbControllerCreate(
        MaaToolKitGetDeviceAdbPath(kIndex), MaaToolKitGetDeviceAdbSerial(kIndex),
        MaaToolKitGetDeviceAdbControllerType(kIndex), MaaToolKitGetDeviceAdbConfig(kIndex), nullptr, nullptr);
    auto ctrl_id = MaaControllerPostConnection(controller_handle);
    MaaControllerWait(controller_handle, ctrl_id);

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto resource_dir = std::filesystem::path(argv[0]).parent_path() / "resource";
    auto res_id = MaaResourcePostResource(resource_handle, resource_dir.string().c_str());
    MaaResourceWait(resource_handle, res_id);

    auto maa_handle = MaaCreate(nullptr, nullptr);
    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    if (!MaaInited(maa_handle)) {
        std::cout << "failed to init maa" << std::endl;

        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
        MaaToolKitUninit();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, "StartUp", MaaTaskParam_Empty);
    MaaWaitTask(maa_handle, task_id);

    MaaDestroy(maa_handle);
    MaaResourceDestroy(resource_handle);
    MaaControllerDestroy(controller_handle);
    MaaToolKitUninit();

    return 0;
}
