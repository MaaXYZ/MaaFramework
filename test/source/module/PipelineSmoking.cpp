#include "PipelineSmoking.h"

#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"

bool pipeline_smoking(const std::filesystem::path& testset_dir)
{
    auto testing_path = testset_dir / "PipelineSmoking";
    auto result_path = testset_dir / "debug";
    json::value debug_config = { { "device_info", json::object {
                                                      { "uuid", testing_path.string() },
                                                      { "screen_width", 1280 },
                                                      { "screen_height", 720 },
                                                      { "orientation", 0 },
                                                  } } };
    auto config = debug_config.dumps(4);

    auto controller_handle =
        MaaDbgControllerCreate(testing_path.string().c_str(), result_path.string().c_str(),
                                     MaaDbgControllerType_Screencap_ReadIndex, config.c_str(), nullptr, nullptr);

    auto ctrl_id = MaaControllerPostConnection(controller_handle);

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);
    auto resource_dir = testset_dir / "PipelineSmoking" / "resource";
    auto res_id = MaaResourcePostPath(resource_handle, resource_dir.string().c_str());

    MaaControllerWait(controller_handle, ctrl_id);
    MaaResourceWait(resource_handle, res_id);

    auto maa_handle = MaaCreate(nullptr, nullptr);
    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    auto destroy = [&]() {
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    };

    if (!MaaInited(maa_handle)) {
        std::cout << __FUNCTION__ << " | Failed to init" << std::endl;

        destroy();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, "Wilderness", MaaTaskParam_Empty);
    MaaWaitTask(maa_handle, task_id);

    destroy();

    return true;
}
