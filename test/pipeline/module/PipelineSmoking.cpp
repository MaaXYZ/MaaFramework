#include "PipelineSmoking.h"

#include "MaaFramework/MaaAPI.h"

bool pipeline_smoking(const std::filesystem::path& testset_dir)
{
    auto testing_path = testset_dir / "PipelineSmoking" / "MaaRecording.txt";
    auto result_path = testset_dir / "debug";

    auto controller_handle = MaaDbgControllerCreate(
        testing_path.string().c_str(),
        result_path.string().c_str(),
        MaaDbgControllerType_ReplayRecording,
        MaaTaskParam_Empty,
        nullptr,
        nullptr);

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
        destroy();
        return false;
    }

    auto task_id = MaaPostTask(maa_handle, "Wilderness", MaaTaskParam_Empty);
    auto status = MaaWaitTask(maa_handle, task_id);

    destroy();

    return status == MaaStatus_Success;
}