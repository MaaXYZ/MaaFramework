#include "CustomRecognizer.h"

#include "Utils/NoWarningCV.hpp"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Task/Context.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomRecognizer::CustomRecognizer(
    const cv::Mat& image,
    const cv::Rect& roi,
    const MAA_VISION_NS::CustomRecognizerParam& param,
    MAA_RES_NS::CustomRecognizerSession session,
    Context& context,
    std::string name)
    : VisionBase(image, roi, name)
    , param_(std::move(param))
    , session_(std::move(session))
    , context_(context)
{
    analyze();
}

void CustomRecognizer::analyze()
{
    LogFunc << VAR(context_.task_id()) << VAR(name_) << VAR_VOIDP(session_.recoginzer) << VAR_VOIDP(session_.trans_arg)
            << VAR(param_.custom_param);

    if (!session_.recoginzer) {
        LogError << "recognizer is null" << VAR(name_) << VAR(param_.name);
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    /*in*/
    ImageBuffer image_buffer(image_);
    MaaRect rect_buf { .x = roi_.x, .y = roi_.y, .width = roi_.width, .height = roi_.height };
    std::string custom_param_str = param_.custom_param.to_string();

    /*out*/
    MaaRect cbox { 0 };
    StringBuffer detail_buffer;

    bool ret = session_.recoginzer(
        &context_,
        context_.task_id(),
        name_.c_str(),
        param_.name.c_str(),
        custom_param_str.c_str(),
        &image_buffer,
        &rect_buf,
        session_.trans_arg,
        &cbox,
        &detail_buffer);

    cv::Rect box { cbox.x, cbox.y, cbox.width, cbox.height };
    std::string detail(detail_buffer.data(), detail_buffer.size());

    auto jdetail = json::parse(detail).value_or(detail);
    Result res { .box = box, .detail = std::move(jdetail) };

    all_results_ = { res };
    if (ret) {
        filtered_results_ = { res };
        best_result_ = res;
    }

    auto cost = duration_since(start_time);
    LogTrace << VAR(name_) << VAR(param_.name) << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost)
             << VAR(ret);
}

MAA_TASK_NS_END
