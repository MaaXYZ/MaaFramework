#include "Classifier.h"

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Utils/NoWarningCV.hpp"
#include "Utils/Ranges.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Classifier::ResultOpt Classifier::analyze() const
{
    LogFunc << name_;

    if (!param_.session) {
        LogError << "OrtSession not loaded";
        return std::nullopt;
    }
    if (param_.cls_size == 0) {
        LogError << "cls_size == 0";
        return std::nullopt;
    }
    if (param_.cls_size != param_.labels.size()) {
        LogError << "cls_size != labels.size()" << VAR(param_.cls_size) << VAR(param_.labels.size());
        return std::nullopt;
    }
    auto start = std::chrono::steady_clock::now();

    Result result = foreach_rois();

    auto costs = duration_since(start);
    LogDebug << name_ << result.box << VAR(result.cls_index) << VAR(result.label) << VAR(result.score) << VAR(costs);

    return result.score == 0 ? std::nullopt : std::make_optional(result);
}

Classifier::Result Classifier::foreach_rois() const
{
    if (!cache_.empty()) {
        return classify(cache_);
    }

    if (param_.roi.empty()) {
        return classify(cv::Rect(0, 0, image_.cols, image_.rows));
    }

    for (const cv::Rect& roi : param_.roi) {
        Result res = classify(roi);
        auto find_it = MAA_RNS::ranges::find(param_.expected, res.cls_index);
        if (find_it == param_.expected.end()) {
            continue;
        }
        return res;
    }

    return {};
}

Classifier::Result Classifier::classify(const cv::Rect& roi) const
{
    if (!param_.session) {
        LogError << "OrtSession not loaded";
        return {};
    }

    cv::Mat image = image_with_roi(roi);
    std::vector<float> input = image_to_tensor(image);

    // TODO: GPU
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t kBatchSize = 1;
    std::array<int64_t, 4> input_shape { kBatchSize, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(),
                                                              input_shape.data(), input_shape.size());
    Result result;
    result.raw.resize(param_.cls_size);

    std::array<int64_t, 2> output_shape { kBatchSize, static_cast<int64_t>(param_.cls_size) };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, result.raw.data(), result.raw.size(),
                                                               output_shape.data(), output_shape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_0 = param_.session->GetInputNameAllocated(0, allocator).get();
    const std::string out_0 = param_.session->GetOutputNameAllocated(0, allocator).get();
    const std::vector input_names { in_0.c_str() };
    const std::vector output_names { out_0.c_str() };

    Ort::RunOptions run_options;
    param_.session->Run(run_options, input_names.data(), &input_tensor, 1, output_names.data(), &output_tensor, 1);

    result.probs = softmax(result.raw);
    result.cls_index = std::max_element(result.probs.begin(), result.probs.end()) - result.probs.begin();
    result.score = result.probs[result.cls_index];
    result.label = param_.labels[result.cls_index];
    result.box = roi;

    LogDebug << VAR(roi) << VAR(result.cls_index) << VAR(result.label) << VAR(result.score) << VAR(result.probs)
             << VAR(result.raw);

    draw_result(result);

    return result;
}

void Classifier::draw_result(const Result& result) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(result.box);
    cv::Point pt(result.box.x + result.box.width + 5, result.box.y);

    for (size_t i = 0; i != param_.cls_size; ++i) {
        const auto color = i == result.cls_index ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
        std::string text =
            MAA_FMT::format("{} {}: prob {:.3f}, raw {:.3f}", i, param_.labels[i], result.probs[i], result.raw[i]);
        cv::putText(image_draw, text, pt, cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
        pt.y += 10;
    }

    if (save_draw_) {
        save_image(image_draw);
    }
}

MAA_VISION_NS_END
