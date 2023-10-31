#include "RunWithoutFile.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"

bool run_without_file(const std::filesystem::path& image_dir)
{
    auto controller_handle = MaaDbgControllerCreate(image_dir.string().c_str(), "", MaaDbgControllerType_CarouselImage,
                                                    MaaTaskParam_Empty, nullptr, nullptr);

    MaaControllerWait(controller_handle, MaaControllerPostConnection(controller_handle));

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);

    auto maa_handle = MaaCreate(nullptr, nullptr);
    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    json::value diff_task { { "MyTask", json::object {} } };
    json::value task_param { { "diff_task", diff_task } };
    std::string task_param_str = task_param.to_string();

    auto task_id = MaaPostTask(maa_handle, "MyTask", task_param_str.c_str());
    auto status = MaaWaitTask(maa_handle, task_id);

    MaaDestroy(maa_handle);
    MaaResourceDestroy(resource_handle);
    MaaControllerDestroy(controller_handle);

    return status == MaaStatus_Success;
}
