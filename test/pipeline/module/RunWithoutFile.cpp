#include "RunWithoutFile.h"

#include <meojson/json.hpp>

#include <cstring>
#include <iostream>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"

#ifdef _MSC_VER
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

MaaBool my_action(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);

struct ActionFailedCapture
{
    bool seen = false;
    MaaActId action_id = MaaInvalidId;
    std::string name;
    std::string action;
    bool success = true;
};

void capture_action_failed(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;

    if (!message || std::strcmp(message, MaaMsg_Node_Action_Failed) != 0 || !details_json || !trans_arg) {
        return;
    }

    auto parsed = json::parse(details_json);
    if (!parsed || !parsed->contains("action_details")) {
        return;
    }

    const auto& action_details = parsed->at("action_details");
    if (!action_details.contains("action_id") || !action_details.contains("name") || !action_details.contains("action")
        || !action_details.contains("success")) {
        return;
    }

    auto* capture = static_cast<ActionFailedCapture*>(trans_arg);
    capture->seen = true;
    capture->action_id = static_cast<MaaActId>(action_details.at("action_id").as_integer());
    capture->name = action_details.at("name").as_string();
    capture->action = action_details.at("action").as_string();
    capture->success = action_details.at("success").as_boolean();
}

bool run_without_file(const std::filesystem::path& testset_dir)
{
    auto screenshot_path = testset_dir / "PipelineSmoking" / "Screenshot";

    auto controller_handle = MaaDbgControllerCreate(screenshot_path.string().c_str());

    MaaControllerWait(controller_handle, MaaControllerPostConnection(controller_handle));

    auto resource_handle = MaaResourceCreate();

    auto tasker_handle = MaaTaskerCreate();
    MaaTaskerBindResource(tasker_handle, resource_handle);
    MaaTaskerBindController(tasker_handle, controller_handle);

    {
        ActionFailedCapture capture;
        auto sink_id = MaaTaskerAddContextSink(tasker_handle, &capture_action_failed, &capture);

        auto box = MaaRectCreate();
        MaaRectSet(box, 0, 0, 0, 0);

        auto failed_id = MaaTaskerPostAction(tasker_handle, "Click", "{}", box, "{}");
        MaaTaskerWait(tasker_handle, failed_id);

        MaaRectDestroy(box);

        MaaTaskerRemoveContextSink(tasker_handle, sink_id);

        if (failed_id == MaaInvalidId || !capture.seen || capture.action_id == MaaInvalidId || capture.name.empty()
            || capture.action != "Click" || capture.success) {
            std::cout << "Failed to preserve action detail on failed action" << std::endl;
            return false;
        }
    }

    {
        auto failed_id = MaaTaskerPostTask(tasker_handle, "_NotExists_", "{}");
        auto failed_status = MaaTaskerWait(tasker_handle, failed_id);
        if (failed_id == MaaInvalidId || failed_status != MaaStatus_Failed) {
            std::cout << "Failed to detect invalid task" << std::endl;
            return false;
        }
    }

    MaaResourceRegisterCustomAction(resource_handle, "MyAct", &my_action, nullptr);

    json::value task_param {
        { "MyTask", json::object { { "action", "Custom" }, { "custom_action", "MyAct" }, { "custom_action_param", "abcdefg" } } }
    };
    std::string task_param_str = task_param.to_string();

    auto task_id = MaaTaskerPostTask(tasker_handle, "MyTask", task_param_str.c_str());
    auto status = MaaTaskerWait(tasker_handle, task_id);

    MaaTaskerDestroy(tasker_handle);
    MaaResourceDestroy(resource_handle);
    MaaControllerDestroy(controller_handle);

    return status == MaaStatus_Succeeded;
}

MaaBool my_action(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    auto image_buffer = MaaImageBufferCreate();
    auto tasker = MaaContextGetTasker(context);
    auto controller = MaaTaskerGetController(tasker);
    MaaControllerCachedImage(controller, image_buffer);

    auto out_box = MaaRectCreate();
    auto out_detail = MaaStringBufferCreate();

    json::value pp_override { { "MyColorMatching",
                                json::object {
                                    { "recognition", "ColorMatch" },
                                    { "lower", json::array { 100, 100, 100 } },
                                    { "upper", json::array { 255, 255, 255 } },
                                } } };
    std::string pp_override_str = pp_override.to_string();

    MaaRecoId my_reco_id = MaaContextRunRecognition(context, "MyColorMatching", pp_override_str.c_str(), image_buffer);
    MaaTaskerGetRecognitionDetail(tasker, my_reco_id, nullptr, nullptr, nullptr, out_box, out_detail, nullptr, nullptr);

    auto detail_string = MaaStringBufferGet(out_detail);
    std::ignore = detail_string;

    MaaImageBufferDestroy(image_buffer);
    MaaRectDestroy(out_box);
    MaaStringBufferDestroy(out_detail);

    return true;
}
