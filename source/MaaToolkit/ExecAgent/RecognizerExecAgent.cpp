#include "RecognizerExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/Time.hpp"

MAA_TOOLKIT_NS_BEGIN

RecognizerExecAgent::RecognizerExecAgent()
{
    custom_recognizer_.analyze = &RecognizerExecAgent::maa_api_analyze;
}

bool RecognizerExecAgent::register_for_maa_inst(
    MaaInstanceHandle handle,
    std::string_view name,
    ExecData& executor)
{
    return MaaRegisterCustomRecognizer(
        handle,
        name.data(),
        &custom_recognizer_,
        reinterpret_cast<void*>(&executor));
}

bool RecognizerExecAgent::unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaUnregisterCustomRecognizer(handle, name.data());
}

std::optional<RecognizerExecAgent::AnalyzeResult> RecognizerExecAgent::analyze(
    ExecData& data,
    MaaSyncContextHandle sync_context,
    const cv::Mat& image,
    std::string_view task_name,
    std::string_view custom_recognition_param)
{
    LogFunc << VAR(data.name) << VAR_VOIDP(sync_context) << VAR(image.size()) << VAR(task_name)
            << VAR(custom_recognition_param);

    std::string handle_arg = arg_cvt_.sync_context_to_arg(sync_context);
    std::string image_arg = arg_cvt_.image_to_arg(image);

    std::vector<std::string> extra_args = { handle_arg,
                                            image_arg,
                                            std::string(task_name),
                                            std::string(custom_recognition_param) };
    std::vector<std::string> args = data.exec_args;
    args.insert(
        args.end(),
        std::make_move_iterator(extra_args.begin()),
        std::make_move_iterator(extra_args.end()));

    auto ret_opt = run_executor(data.exec_path, args);
    if (!ret_opt) {
        LogError << "run_executor failed" << VAR(data.exec_path) << VAR(data.exec_args);
        return std::nullopt;
    }
    auto& ret = *ret_opt;
    LogTrace << VAR(ret);

    AnalyzeResult result;

    auto box_vec = ret.get("box", std::vector<int>());
    if (box_vec.size() == 4) {
        result.box = cv::Rect(box_vec[0], box_vec[1], box_vec[2], box_vec[3]);
    }
    auto jdetail = ret.get("detail", json::value());
    result.detail = jdetail.is_string() ? jdetail.as_string() : jdetail.to_string();

    return result;
}

MaaBool RecognizerExecAgent::maa_api_analyze(
    MaaSyncContextHandle sync_context,
    const MaaImageBufferHandle image,
    MaaStringView task_name,
    MaaStringView custom_recognition_param,
    MaaTransparentArg recognizer_arg,
    MaaRectHandle out_box,
    MaaStringBufferHandle out_detail)
{
    auto* data = static_cast<ExecData*>(recognizer_arg);
    if (!data) {
        LogError << "data is nullptr" << VAR(recognizer_arg);
        return false;
    }

    void* raw_data = MaaGetImageRawData(image);
    int32_t width = MaaGetImageWidth(image);
    int32_t height = MaaGetImageHeight(image);
    int32_t type = MaaGetImageType(image);
    cv::Mat image_mat(height, width, type, raw_data);

    auto result_opt =
        get_instance().analyze(*data, sync_context, image_mat, task_name, custom_recognition_param);

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