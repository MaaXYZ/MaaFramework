#include "RecognizerExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

RecognizerExecAgent::RecognizerExecAgent()
{
    custom_recognizer_.analyze = &RecognizerExecAgent::maa_api_analyze;
}

bool RecognizerExecAgent::register_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaRegisterCustomRecognizer(handle, name.data(), &custom_recognizer_, this);
}

bool RecognizerExecAgent::unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaUnregisterCustomRecognizer(handle, name.data());
}

std::optional<RecognizerExecAgent::AnalyzeResult> RecognizerExecAgent::analyze(
    MaaSyncContextHandle sync_context, const cv::Mat& image, std::string_view task_name,
    std::string_view custom_recognition_param)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(image.size()) << VAR(task_name) << VAR(custom_recognition_param);

    auto exec_it = executors_.find(task_name.data());
    if (exec_it == executors_.end()) {
        LogError << "no executor found for task: " << task_name;
        return std::nullopt;
    }
    auto& executor = exec_it->second;
}

MaaBool RecognizerExecAgent::maa_api_analyze( //
    MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
    MaaStringView custom_recognition_param, MaaTransparentArg recognizer_arg, MaaRectHandle out_box,
    MaaStringBufferHandle out_detail)
{
    auto* self = static_cast<RecognizerExecAgent*>(recognizer_arg);
    if (!self) {
        LogError << "recognizer_arg is nullptr";
        return false;
    }

    void* raw_data = MaaGetImageRawData(image);
    int32_t width = MaaGetImageWidth(image);
    int32_t height = MaaGetImageHeight(image);
    int32_t type = MaaGetImageType(image);
    cv::Mat image_mat(height, width, type, raw_data);

    auto result_opt = self->analyze(sync_context, image_mat, task_name, custom_recognition_param);

    if (!result_opt) {
        MaaSetRect(out_box, 0, 0, 0, 0);
        MaaClearString(out_detail);
        return false;
    }

    auto& box = result_opt->box;
    MaaSetRect(out_box, box.x, box.y, box.width, box.height);

    auto& detail = result_opt->detail;
    MaaSetStringEx(out_detail, detail.c_str(), detail.size());

    return true;
}

MAA_TOOLKIT_NS_END
