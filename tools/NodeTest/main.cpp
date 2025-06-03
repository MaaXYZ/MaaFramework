#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

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

int main([[maybe_unused]] int argc, char** argv)
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    auto controller_handle = MaaDbgControllerCreate("E:/Downloads/test", "./", MaaDbgControllerType_CarouselImage, "{}", nullptr, nullptr);

    MaaControllerWait(controller_handle, MaaControllerPostConnection(controller_handle));

    auto resource_handle = MaaResourceCreate(nullptr, nullptr);

    auto tasker_handle = MaaTaskerCreate(nullptr, nullptr);
    MaaTaskerBindResource(tasker_handle, resource_handle);
    MaaTaskerBindController(tasker_handle, controller_handle);

    MaaResourceRegisterCustomAction(resource_handle, "MyAct", &my_action, nullptr);

    json::value task_param {
        { "Entry", json::object { { "next", "MyNode" } } },
        { "MyNode", json::object { { "recognition", "FeatureMatch" }, { "template", "E:/Downloads/templ.png" } } },
    };
    std::string task_param_str = task_param.to_string();

    auto task_id = MaaTaskerPostTask(tasker_handle, "Entry", task_param_str.c_str());
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
