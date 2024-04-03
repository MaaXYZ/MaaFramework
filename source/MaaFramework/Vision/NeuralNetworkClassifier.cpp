#include "NeuralNetworkClassifier.h"

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Utils/NoWarningCV.hpp"
#include "VisionUtils.hpp"
#include <ranges>

MAA_VISION_NS_BEGIN

NeuralNetworkClassifier::NeuralNetworkClassifier(
    cv::Mat image,
    NeuralNetworkClassifierParam param,
    std::shared_ptr<Ort::Session> session,
    std::string name)
    : VisionBase(std::move(image), std::move(name))
    , param_(std::move(param))
    , session_(std::move(session))
{
    analyze();
}

void NeuralNetworkClassifier::analyze()
{
    LogFunc << name_ << VAR(uid_);

    if (!session_) {
        LogError << "OrtSession not loaded";
        return;
    }
    if (param_.cls_size == 0) {
        LogError << "cls_size == 0";
        return;
    }
    if (param_.cls_size != param_.labels.size()) {
        LogError << "cls_size != labels.size()" << VAR(param_.cls_size)
                 << VAR(param_.labels.size());
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    auto results = classify_all_rois();
    add_results(std::move(results), param_.expected);

    cherry_pick();

    auto cost = duration_since(start_time);
    LogTrace << name_ << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_)
             << VAR(best_result_) << VAR(cost);
}

NeuralNetworkClassifier::ResultsVec NeuralNetworkClassifier::classify_all_rois() const
{
    if (param_.roi.empty()) {
        return { classify(cv::Rect(0, 0, image_.cols, image_.rows)) };
    }
    else {
        ResultsVec results;
        for (const cv::Rect& roi : param_.roi) {
            Result res = classify(roi);
            results.emplace_back(std::move(res));
        }
        return results;
    }
}

NeuralNetworkClassifier::Result NeuralNetworkClassifier::classify(const cv::Rect& roi) const
{
    if (!session_) {
        LogError << "OrtSession not loaded";
        return {};
    }

    cv::Mat image = image_with_roi(roi);
    std::vector<float> input = image_to_tensor(image);

    // TODO: GPU
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t kBatchSize = 1;
    std::array<int64_t, 4> input_shape { kBatchSize, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input.data(),
        input.size(),
        input_shape.data(),
        input_shape.size());

    std::vector<float> output;
    output.resize(param_.cls_size);
    std::array<int64_t, 2> output_shape { kBatchSize, static_cast<int64_t>(param_.cls_size) };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        output.data(),
        output.size(),
        output_shape.data(),
        output_shape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_0 = session_->GetInputNameAllocated(0, allocator).get();
    const std::string out_0 = session_->GetOutputNameAllocated(0, allocator).get();
    const std::vector input_names { in_0.c_str() };
    const std::vector output_names { out_0.c_str() };

    Ort::RunOptions run_options;
    session_->Run(
        run_options,
        input_names.data(),
        &input_tensor,
        1,
        output_names.data(),
        &output_tensor,
        1);

    Result result;
    result.raw = std::move(output);
    result.probs = softmax(result.raw);
    result.cls_index =
        std::max_element(result.probs.begin(), result.probs.end()) - result.probs.begin();
    result.score = result.probs[result.cls_index];
    result.label = param_.labels[result.cls_index];
    result.box = roi;

    if (debug_draw_) {
        auto draw = draw_result(result);
        handle_draw(draw);
    }

    return result;
}

void NeuralNetworkClassifier::add_results(ResultsVec results, const std::vector<size_t>& expected)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) {
        return std::ranges::find(expected, res.cls_index) != expected.end();
    });

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
    cv::Mat image_draw = draw_roi(res.box);
    cv::Point pt(res.box.x + res.box.width + 5, res.box.y + 20);

    for (size_t i = 0; i != param_.cls_size; ++i) {
        const auto color = i == res.cls_index ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 0, 0);
        std::string text = std::format(
            "{} {}: prob {:.3f}, raw {:.3f}",
            i,
            param_.labels[i],
            res.probs[i],
            res.raw[i]);
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
    case ResultOrderBy::Area:
        sort_by_area_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

MAA_VISION_NS_END