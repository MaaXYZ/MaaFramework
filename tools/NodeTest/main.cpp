#include <iostream>

#include <meojson/json.hpp>
#include <opencv2/opencv.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

MaaBool my_action(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);

MaaBool connect(void*)
{
    return true;
}

MaaBool request_uuid(void* trans_arg, /* out */ MaaStringBuffer* buffer)
{
    MaaStringBufferSet(buffer, "1234567");
    return true;
}

MaaBool screencap(void* trans_arg, /* out */ MaaImageBuffer* buffer)
{
    auto img = cv::imread("test.png");
    MaaImageBufferSetRawData(buffer, img.data, img.cols, img.rows, img.type());
    return true;
}

int main([[maybe_unused]] int argc, char** argv)
{
    std::string user_path = "./";
    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    MaaCustomControllerCallbacks controller {
        .connect = connect,
        .request_uuid = request_uuid,
        .screencap = screencap,
    };

    auto controller_handle = MaaCustomControllerCreate(&controller, nullptr);

    MaaControllerWait(controller_handle, MaaControllerPostConnection(controller_handle));

    auto resource_handle = MaaResourceCreate();

    auto tasker_handle = MaaTaskerCreate();
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
