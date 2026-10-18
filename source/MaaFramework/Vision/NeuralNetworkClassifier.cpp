#include "NeuralNetworkClassifier.h"

#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"
#include <ranges>

MAA_VISION_NS_BEGIN

NeuralNetworkClassifier::NeuralNetworkClassifier(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    NeuralNetworkClassifierParam param,
    InferenceSessionPtr session,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , session_(std::move(session))
{
    analyze();
}

void NeuralNetworkClassifier::analyze()
{
    LogFunc << name_;

    if (!session_) {
        LogError << "InferenceSession not loaded";
        return;
    }
    auto start_time = std::chrono::steady_clock::now();

    init_expected_indices();

    while (next_roi()) {
        auto res = classify();
        add_results({ std::move(res) }, param_.expected);
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model)
             << VAR(param_.labels) << VAR(param_.expected);
}

NeuralNetworkClassifier::Result NeuralNetworkClassifier::classify() const
{
    if (!session_) {
        LogError << "InferenceSession not loaded";
        return { };
    }
    // batch_size, channel, height, width
    // for yolov8, input_shape is { 1, 3, 640, 640 }
    const auto& input_shape = session_->input_shape();
    if (input_shape.size() != 4) {
        LogError << "Input shape is not 4" << VAR(input_shape);
        return { };
    }

    cv::Mat image = image_with_roi();
    cv::Size raw_roi_size(image.cols, image.rows);
    cv::Size input_image_size(static_cast<int>(input_shape[3]), static_cast<int>(input_shape[2]));
    cv::resize(image, image, input_image_size, 0, 0, cv::INTER_AREA);
    std::vector<float> input = image_to_tensor(image);

    auto output_opt = session_->run(input);
    if (!output_opt) {
        LogError << name_ << "inference failed" << VAR(param_.model);
        return { };
    }
    if (output_opt->data.empty()) {
        LogError << name_ << "empty output" << VAR(param_.model) << VAR(output_opt->shape);
        return { };
    }

    std::vector<float> output = std::move(output_opt->data);

    Result res;
    res.raw = std::move(output);
    res.probs = softmax(res.raw);
    res.cls_index = std::max_element(res.probs.begin(), res.probs.end()) - res.probs.begin();
    res.score = res.probs[res.cls_index];
    res.label = res.cls_index < param_.labels.size() ? param_.labels[res.cls_index] : std::format("Unknown_{}", res.cls_index);
    res.box = roi_;

    if (debug_draw_) {
        auto draw = draw_result(res);
        handle_draw(draw);
    }

    return res;
}

void NeuralNetworkClassifier::add_results(ResultsVec results, const std::vector<std::variant<int, std::string>>& expected)
{
    if (expected.empty()) {
        // expected 为空时，所有结果均可用
        merge_vector_(filtered_results_, results);
    }
    else {
        std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) {
            return std::ranges::find(expected_indices_, res.cls_index) != expected_indices_.end();
        });
    }

    merge_vector_(all_results_, std::move(results));
}

void NeuralNetworkClassifier::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

cv::Mat NeuralNetworkClassifier::draw_result(const Result& res) const
{
    cv::Mat image_draw = draw_roi();
    cv::Point pt(res.box.x + res.box.width + 5, res.box.y + 20);

    for (size_t i = 0; i != res.raw.size(); ++i) {
        const auto color = i == res.cls_index ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
        std::string text = std::format("{} {}: prob {:.3f}, raw {:.3f}", i, param_.labels[i], res.probs[i], res.raw[i]);
        cv::putText(image_draw, text, pt, cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
        pt.y += 20;
    }

    return image_draw;
}

void NeuralNetworkClassifier::sort_(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    case ResultOrderBy::Score:
        sort_by_score_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    case ResultOrderBy::Expected:
        sort_by_expected_index_(results, expected_indices_);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

void NeuralNetworkClassifier::init_expected_indices()
{
    expected_indices_.clear();
    expected_indices_.reserve(param_.expected.size());

    for (const auto& item : param_.expected) {
        if (std::holds_alternative<int>(item)) {
            int idx = std::get<int>(item);
            // 校验索引有效性
            if (idx >= 0) {
                expected_indices_.push_back(idx);
            }
            else {
                LogWarn << "Invalid index in expected" << VAR(idx);
            }
        }
        else if (std::holds_alternative<std::string>(item)) {
            const std::string& label = std::get<std::string>(item);
            auto it = std::find(param_.labels.begin(), param_.labels.end(), label);
            if (it != param_.labels.end()) {
                int idx = static_cast<int>(std::distance(param_.labels.begin(), it));
                expected_indices_.push_back(idx);
            }
            else {
                LogWarn << "Label not found in labels list" << VAR(label);
            }
        }
    }
}

MAA_VISION_NS_END
