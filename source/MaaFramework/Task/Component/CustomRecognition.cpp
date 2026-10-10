#include "CustomRecognition.h"

#include <format>

#include "MaaUtils/NoWarningCV.hpp"

#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"
#include "Task/Context.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

namespace
{

enum class MultiResultState
{
    None,
    Valid,
    Invalid,
};

struct MultiResultData
{
    std::vector<CustomRecognitionResult> all;
    std::vector<CustomRecognitionResult> filtered;
};

bool parse_result_entry(const json::value& input, const json::value& top_level_detail, CustomRecognitionResult& output)
{
    if (!input.is_object()) {
        return false;
    }

    auto box = input.find<std::vector<int>>("box");
    if (!box || box->size() != 4) {
        return false;
    }

    output.box = cv::Rect { (*box)[0], (*box)[1], (*box)[2], (*box)[3] };
    // 缺省 detail 记为多结果对象顶层的 detail，没有则为 null，避免把无效值写进结果 JSON
    output.detail = input.get("detail", top_level_detail);
    return true;
}

bool parse_result_list(const json::value& input, const json::value& top_level_detail, std::vector<CustomRecognitionResult>& output)
{
    if (!input.is_array()) {
        return false;
    }

    for (const auto& item : input.as_array()) {
        CustomRecognitionResult res;
        if (!parse_result_entry(item, top_level_detail, res)) {
            return false;
        }
        output.emplace_back(std::move(res));
    }

    return true;
}

// 多结果对象，只有 $all / $filtered / detail 三个保留键会被框架解释，其余 "$" 键视为写错
MultiResultState parse_multi_result(const json::value& detail, MultiResultData& output)
{
    static const std::string kAllKey = "$all";
    static const std::string kFilteredKey = "$filtered";

    if (!detail.is_object()) {
        return MultiResultState::None;
    }

    bool has_all = detail.contains(kAllKey);
    bool has_filtered = detail.contains(kFilteredKey);
    if (!has_all && !has_filtered) {
        return MultiResultState::None;
    }

    for (const auto& iter : detail.as_object()) {
        const std::string& key = iter.first;
        if (key.starts_with("$") && key != kAllKey && key != kFilteredKey) {
            LogError << "unsupported key in multi result" << VAR(key);
            return MultiResultState::Invalid;
        }
    }

    const json::value& top_level_detail = detail.get("detail", json::value(nullptr));

    if (has_all && !parse_result_list(detail.at(kAllKey), top_level_detail, output.all)) {
        LogError << "invalid $all" << VAR(detail);
        return MultiResultState::Invalid;
    }
    if (has_filtered && !parse_result_list(detail.at(kFilteredKey), top_level_detail, output.filtered)) {
        LogError << "invalid $filtered" << VAR(detail);
        return MultiResultState::Invalid;
    }

    if (!has_all) {
        output.all = output.filtered;
    }
    if (!has_filtered) {
        output.filtered = output.all;
    }

    return MultiResultState::Valid;
}

} // namespace

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

    if (ret) {
        MultiResultData multi_results;
        switch (parse_multi_result(jdetail, multi_results)) {
        case MultiResultState::Valid: {
            all_results_ = std::move(multi_results.all);
            filtered_results_ = std::move(multi_results.filtered);
            if (auto index_opt = MAA_VISION_NS::pythonic_index(filtered_results_.size(), param_.result_index)) {
                best_result_ = filtered_results_.at(*index_opt);
            }
        } break;

        case MultiResultState::Invalid:
            // 多结果对象非法时不猜结果，视为本次识别无命中
            break;

        case MultiResultState::None: {
            Result res { .box = box, .detail = std::move(jdetail) };
            all_results_ = { res };
            filtered_results_ = { res };
            best_result_ = res;
        } break;
        }
    }
    else {
        all_results_ = { Result { .box = box, .detail = std::move(jdetail) } };
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
