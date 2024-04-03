#include "RunWithoutFile.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"

void register_my_action(MaaInstanceHandle maa_handle);

bool run_without_file(const std::filesystem::path& testset_dir)
{
    auto testing_path = testset_dir / "PipelineSmoking" / "Screenshot";
    auto result_path = testset_dir / "debug";

    auto controller_handle = MaaDbgControllerCreate(
        testing_path.string().c_str(),
        result_path.string().c_str(),
        MaaDbgControllerType_CarouselImage,
        MaaTaskParam_Empty,
        nullptr,
        nullptr);

    MaaControllerWait(controller_handle, MaaControllerPostConnection(controller_handle));

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);

    auto maa_handle = MaaCreate(nullptr, nullptr);
    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    register_my_action(maa_handle);

    json::value task_param { { "MyTask",
                               json::object { { "action", "Custom" },
                                              { "custom_action", "MyAct" },
                                              { "custom_action_param", "abcdefg" } } } };
    std::string task_param_str = task_param.to_string();

    auto task_id = MaaPostTask(maa_handle, "MyTask", task_param_str.c_str());
    auto status = MaaWaitTask(maa_handle, task_id);

    MaaDestroy(maa_handle);
    MaaResourceDestroy(resource_handle);
    MaaControllerDestroy(controller_handle);

    return status == MaaStatus_Success;
}

MaaBool my_action_run(
    [[maybe_unused]] MaaSyncContextHandle sync_context,
    [[maybe_unused]] MaaStringView task_name,
    [[maybe_unused]] MaaStringView custom_action_param,
    [[maybe_unused]] MaaRectHandle cur_box,
    [[maybe_unused]] MaaStringView cur_rec_detail,
    [[maybe_unused]] MaaTransparentArg action_arg)
{
    auto image = MaaCreateImageBuffer();
    MaaSyncContextScreencap(sync_context, image);

    auto out_box = MaaCreateRectBuffer();
    auto out_detail = MaaCreateStringBuffer();

    json::value task_param { { "MyColorMatching",
                               json::object {
                                   { "recognition", "ColorMatch" },
                                   { "lower", json::array { 100, 100, 100 } },
                                   { "upper", json::array { 255, 255, 255 } },
                               } } };
    std::string task_param_str = task_param.to_string();

    MaaSyncContextRunRecognizer(
        sync_context,
        image,
        "MyColorMatching",
        task_param_str.c_str(),
        out_box,
        out_detail);

    auto detail_string = MaaGetString(out_detail);
    std::ignore = detail_string;

    MaaDestroyImageBuffer(image);
    MaaDestroyRectBuffer(out_box);
    MaaDestroyStringBuffer(out_detail);

    return true;
}

MaaCustomActionAPI my_action {};

void register_my_action(MaaInstanceHandle maa_handle)
{
    my_action.run = my_action_run;
    MaaRegisterCustomAction(maa_handle, "MyAct", &my_action, nullptr);
}