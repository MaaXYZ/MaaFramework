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

    auto exec_it = executors_.find(std::string(task_name));
    if (exec_it == executors_.end()) {
        LogError << "executor not found" << VAR(task_name);
        return std::nullopt;
    }
    auto& exec = exec_it->second;

    std::string handle_arg = arg_cvt_.sync_context_to_arg(sync_context);
    std::string image_arg = arg_cvt_.image_to_arg(exec.image_mode, image);

    std::vector<std::string> extra_args = { handle_arg, image_arg, std::string(task_name),
                                            std::string(custom_recognition_param) };
    std::vector<std::string> args = exec.exec_args;
    args.insert(args.end(), std::make_move_iterator(extra_args.begin()), std::make_move_iterator(extra_args.end()));

    auto output_opt = run_executor(exec.text_mode, exec.exec_path, args);
    if (!output_opt) {
        LogError << "run_executor failed" << VAR(exec.exec_path) << VAR(exec.exec_args);
        return std::nullopt;
    }

    // TODO: parse output to get result
    return {};
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
