#include "RunWithoutFile.h"

#include <meojson/json.hpp>

#include <iostream>

#include "MaaFramework/MaaAPI.h"

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

MaaBool match_second_candidate(
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
        auto failed_id = MaaTaskerPostTask(tasker_handle, "_NotExists_", "{}");
        auto failed_status = MaaTaskerWait(tasker_handle, failed_id);
        if (failed_id == MaaInvalidId || failed_status != MaaStatus_Failed) {
            std::cout << "Failed to detect invalid task" << std::endl;
            return false;
        }
    }

    MaaResourceRegisterCustomAction(resource_handle, "MyAct", &my_action, nullptr);
    int recognition_attempts = 0;
    MaaResourceRegisterCustomRecognition(resource_handle, "MatchSecondCandidate", &match_second_candidate, &recognition_attempts);

    json::value backtracking_param {
        { "Backtracking",
          json::object {
              { "recognition", "And" },
              { "all_of",
                json::array {
                    json::object {
                        { "sub_name", "candidate" },
                        { "recognition", "Or" },
                        { "any_of",
                          json::array {
                              json::object {
                                  { "sub_name", "A" },
                                  { "recognition", "DirectHit" },
                                  { "roi", json::array { 10, 10, 20, 20 } },
                              },
                              json::object {
                                  { "sub_name", "B" },
                                  { "recognition", "DirectHit" },
                                  { "roi", json::array { 200, 10, 20, 20 } },
                              },
                          } },
                    },
                    json::object {
                        { "recognition", "Custom" },
                        { "custom_recognition", "MatchSecondCandidate" },
                        { "roi", "candidate" },
                    },
                } },
          } },
    };
    std::string backtracking_param_str = backtracking_param.to_string();
    auto backtracking_id = MaaTaskerPostTask(tasker_handle, "Backtracking", backtracking_param_str.c_str());
    auto backtracking_status = MaaTaskerWait(tasker_handle, backtracking_id);
    MaaNodeId backtracking_node_id = MaaInvalidId;
    MaaSize node_id_list_size = 1;
    MaaTaskerGetTaskDetail(
        tasker_handle,
        backtracking_id,
        nullptr,
        &backtracking_node_id,
        &node_id_list_size,
        nullptr);
    MaaRecoId backtracking_reco_id = MaaInvalidId;
    MaaTaskerGetNodeDetail(tasker_handle, backtracking_node_id, nullptr, &backtracking_reco_id, nullptr, nullptr);
    auto backtracking_box = MaaRectCreate();
    MaaTaskerGetRecognitionDetail(tasker_handle, backtracking_reco_id, nullptr, nullptr, nullptr, backtracking_box, nullptr, nullptr, nullptr);
    bool backtracking_succeeded =
        backtracking_status == MaaStatus_Succeeded && recognition_attempts == 2 && MaaRectGetX(backtracking_box) == 200;
    MaaRectDestroy(backtracking_box);
    if (!backtracking_succeeded) {
        std::cout << "Failed to backtrack nested Or recognition" << std::endl;
        return false;
    }

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

MaaBool match_second_candidate(
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
    ++*static_cast<int*>(trans_arg);
    if (MaaRectGetX(roi) != 200) {
        return false;
    }

    MaaRectSet(out_box, MaaRectGetX(roi), MaaRectGetY(roi), MaaRectGetW(roi), MaaRectGetH(roi));
    MaaStringBufferSet(out_detail, "{}");
    return true;
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
