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

MaaBool my_recognition(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    MaaRect* out_box,
    MaaStringBuffer* out_detail);

struct ActionFailedCapture
{
    bool seen = false;
    MaaTaskId task_id = MaaInvalidId;
    MaaActId action_id = MaaInvalidId;
    std::string name;
    std::string action;
    int32_t box_x = 0;
    int32_t box_y = 0;
    int32_t box_w = 0;
    int32_t box_h = 0;
    bool success = true;
};

void capture_action_failed(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;

    if (!message || std::strcmp(message, MaaMsg_Node_Action_Failed) != 0 || !details_json || !trans_arg) {
        return;
    }

    auto parsed = json::parse(details_json);
    if (!parsed || !parsed->contains("task_id") || !parsed->contains("action_details")) {
        return;
    }

    const auto& action_details = parsed->at("action_details");
    if (!action_details.contains("action_id") || !action_details.contains("name") || !action_details.contains("action")
        || !action_details.contains("box") || !action_details.contains("success")) {
        return;
    }

    auto* capture = static_cast<ActionFailedCapture*>(trans_arg);
    capture->task_id = static_cast<MaaTaskId>(parsed->at("task_id").as_integer());
    capture->seen = true;
    capture->action_id = static_cast<MaaActId>(action_details.at("action_id").as_integer());
    capture->name = action_details.at("name").as_string();
    capture->action = action_details.at("action").as_string();
    const auto& box = action_details.at("box").as_array();
    if (box.size() == 4) {
        capture->box_x = static_cast<int32_t>(box[0].as_integer());
        capture->box_y = static_cast<int32_t>(box[1].as_integer());
        capture->box_w = static_cast<int32_t>(box[2].as_integer());
        capture->box_h = static_cast<int32_t>(box[3].as_integer());
    }
    capture->success = action_details.at("success").as_boolean();
}

bool check_multi_recognition(MaaTasker* tasker, MaaController* controller, int32_t index, bool expect_hit, int32_t expect_x)
{
    auto image_buffer = MaaImageBufferCreate();
    MaaControllerCachedImage(controller, image_buffer);

    json::value param {
        { "custom_recognition", "MultiRec" },
        { "index", index },
    };
    std::string param_str = param.to_string();

    auto task_id = MaaTaskerPostRecognition(tasker, "Custom", param_str.c_str(), image_buffer);
    MaaTaskerWait(tasker, task_id);
    MaaImageBufferDestroy(image_buffer);

    MaaSize node_size = 0;
    if (task_id == MaaInvalidId || !MaaTaskerGetTaskDetail(tasker, task_id, nullptr, nullptr, &node_size, nullptr) || node_size != 1) {
        std::cout << "Failed to run custom recognition with index " << index << std::endl;
        return false;
    }

    MaaNodeId node_id = MaaInvalidId;
    if (!MaaTaskerGetTaskDetail(tasker, task_id, nullptr, &node_id, &node_size, nullptr)) {
        std::cout << "Failed to get node id" << std::endl;
        return false;
    }

    MaaRecoId reco_id = MaaInvalidId;
    MaaBool completed = false;
    if (!MaaTaskerGetNodeDetail(tasker, node_id, nullptr, &reco_id, nullptr, &completed) || reco_id == MaaInvalidId) {
        std::cout << "Failed to get node detail" << std::endl;
        return false;
    }

    auto out_box = MaaRectCreate();
    auto out_detail = MaaStringBufferCreate();
    MaaBool hit = false;
    bool got = MaaTaskerGetRecognitionDetail(tasker, reco_id, nullptr, nullptr, &hit, out_box, out_detail, nullptr, nullptr);
    MaaRectDestroy(out_box);
    auto parsed = json::parse(MaaStringBufferGet(out_detail));
    MaaStringBufferDestroy(out_detail);

    if (!got || !parsed) {
        std::cout << "Failed to get recognition detail" << std::endl;
        return false;
    }

    const auto& detail = *parsed;
    if (detail.at("all").as_array().size() != 2 || detail.at("filtered").as_array().size() != 2) {
        std::cout << "Unexpected multi result count with index " << index << std::endl;
        return false;
    }

    // 元素缺省 detail 时使用多结果对象顶层的 detail
    if (detail.at("all").as_array()[0].at("detail").as_string() != "shared"
        || detail.at("all").as_array()[1].at("detail").as_string() != "second") {
        std::cout << "Unexpected multi result detail with index " << index << std::endl;
        return false;
    }

    bool hit_matched = (hit != 0) == expect_hit && (completed != 0) == expect_hit;
    bool best_matched = expect_hit ? (detail.at("best").at("box").as_array()[0].as_integer() == expect_x) : detail.at("best").is_null();
    if (!hit_matched || !best_matched) {
        std::cout << "Multi result index selection mismatch with index " << index << std::endl;
        return false;
    }

    return true;
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
        auto no_controller_tasker_handle = MaaTaskerCreate();
        MaaTaskerBindResource(no_controller_tasker_handle, resource_handle);

        ActionFailedCapture capture;
        auto sink_id = MaaTaskerAddContextSink(no_controller_tasker_handle, &capture_action_failed, &capture);

        auto box = MaaRectCreate();
        MaaRectSet(box, 10, 20, 30, 40);

        auto failed_id = MaaTaskerPostAction(no_controller_tasker_handle, "Click", R"({"target":[1,2,3,4]})", box, "{}");
        MaaTaskerWait(no_controller_tasker_handle, failed_id);

        MaaRectDestroy(box);

        MaaTaskerRemoveContextSink(no_controller_tasker_handle, sink_id);
        MaaTaskerDestroy(no_controller_tasker_handle);

        if (failed_id == MaaInvalidId || !capture.seen || capture.task_id != failed_id || capture.action_id == MaaInvalidId
            || capture.name.empty() || capture.action != "Click" || capture.box_x != 10 || capture.box_y != 20 || capture.box_w != 30
            || capture.box_h != 40 || capture.success) {
            std::cout << "Failed to preserve or correctly associate action detail on failed action" << std::endl;
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

    MaaResourceRegisterCustomRecognition(resource_handle, "MultiRec", &my_recognition, nullptr);

    MaaControllerWait(controller_handle, MaaControllerPostScreencap(controller_handle));

    // index 在回调给出的顺序上选取，越界视为无结果
    if (!check_multi_recognition(tasker_handle, controller_handle, -1, true, 21)
        || !check_multi_recognition(tasker_handle, controller_handle, 5, false, 0)) {
        return false;
    }

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

MaaBool my_recognition(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    MaaRect* out_box,
    MaaStringBuffer* out_detail)
{
    std::ignore = context;
    std::ignore = task_id;
    std::ignore = node_name;
    std::ignore = custom_recognition_name;
    std::ignore = custom_recognition_param;
    std::ignore = image;
    std::ignore = roi;
    std::ignore = trans_arg;

    MaaRectSet(out_box, 11, 12, 13, 14);
    MaaStringBufferSet(out_detail, R"({"detail":"shared","$all":[{"box":[11,12,13,14]},{"box":[21,22,23,24],"detail":"second"}]})");

    return true;
}
