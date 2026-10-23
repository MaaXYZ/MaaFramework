#include "CustomRecognition.h"

#include <format>

#include "MaaUtils/NoWarningCV.hpp"

#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"
#include "Task/Context.h"

MAA_TASK_NS_BEGIN

CustomRecognition::CustomRecognition(
    const cv::Mat& image,
    const cv::Rect& roi,
    const MAA_VISION_NS::CustomRecognitionParam& param,
    MAA_RES_NS::CustomRecognitionSession session,
    Context& context,
    std::string name)
    : VisionBase(image, { roi }, name)
    , param_(param)
    , session_(std::move(session))
    , context_(context)
{
    analyze();
}

void CustomRecognition::analyze()
{
    LogFunc << VAR(context_.task_id()) << VAR(name_) << VAR_VOIDP(session_.recognition) << VAR_VOIDP(session_.trans_arg) << VAR(param_.name)
            << VAR(param_.custom_param);

    if (!session_.recognition) {
        LogError << "recognition is null" << VAR(name_) << VAR(param_.name);
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    next_roi();

    /*in*/
    ImageBuffer image_buffer(image_);
    MaaRect rect_buf { .x = roi_.x, .y = roi_.y, .width = roi_.width, .height = roi_.height };
    std::string custom_param_str = param_.custom_param.to_string();

    /*out*/
    MaaRect cbox { 0 };
    StringBuffer detail_buffer;

    bool ret = session_.recognition(
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
    const std::string& detail = detail_buffer.get();

    auto jdetail = json::parse(detail).value_or(detail);
    Result res { .box = box, .detail = std::move(jdetail) };

    all_results_ = { res };
    if (ret) {
        filtered_results_ = { res };
        best_result_ = res;
    }

    if (debug_draw_ && !image_.empty()) {
        handle_draw(draw_result(res, ret));
    }

    auto cost = duration_since(start_time);
    LogDebug << VAR(name_) << VAR(param_.name) << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(ret);
}

cv::Mat CustomRecognition::draw_result(const Result& res, bool hit) const
{
    cv::Mat image_draw = draw_roi();
    const auto color = hit ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
    const bool has_box = res.box.width > 0 && res.box.height > 0;

    std::string flag = std::format("{}: {}", hit ? "hit" : "miss", param_.name);
    cv::Point origin(5, 25);
    if (has_box) {
        flag += std::format(", [{}, {}, {}, {}]", res.box.x, res.box.y, res.box.width, res.box.height);
        cv::rectangle(image_draw, res.box, color, 1);
        origin = cv::Point(res.box.x, res.box.y - 5);
    }

    cv::putText(image_draw, flag, origin, cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    return image_draw;
}

MAA_TASK_NS_END
