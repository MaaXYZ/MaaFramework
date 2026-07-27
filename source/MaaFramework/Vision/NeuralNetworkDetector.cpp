#include "NeuralNetworkDetector.h"

#include <algorithm>
#include <array>
#include <format>
#include <numeric>
#include <ranges>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "MaaUtils/NoWarningCV.hpp"
#include "NeuralNetwork/DetectionPostProcessor.h"
#include "NeuralNetwork/ImagePreprocessor.h"
#include "NeuralNetwork/LabelParser.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

NeuralNetworkDetector::NeuralNetworkDetector(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    NeuralNetworkDetectorParam param,
    std::shared_ptr<const NeuralNetwork::ModelPackage> package,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , package_(std::move(package))
{
    analyze();
}

void NeuralNetworkDetector::analyze()
{
    LogFunc << name_;

    if (!package_ || !package_->session || !package_->memory_info || !package_->adapter || package_->input_names.size() != 1) {
        error_ = "detector model package is not loaded";
        LogError << error_;
        return;
    }

    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::string> labels;
    if (!NeuralNetwork::LabelParser::resolve(param_.labels, package_->descriptor.labels, package_->metadata_labels, labels, error_)) {
        LogError << name_ << VAR(error_);
        return;
    }
    NeuralNetwork::DetectionThresholdPolicy threshold_policy;
    if (!NeuralNetwork::DetectionPostProcessor::build_threshold_policy(
            param_.expected,
            param_.thresholds,
            NeuralNetworkDetectorParam::kDefaultThreshold,
            threshold_policy,
            error_)) {
        LogError << name_ << VAR(error_);
        return;
    }

    CandidateResults all_results;
    while (next_roi()) {
        CandidateResults results;
        if (!detect(labels, threshold_policy.score_floor, results, error_)) {
            all_results_.clear();
            filtered_results_.clear();
            best_result_.reset();
            LogError << name_ << VAR(error_);
            return;
        }
        merge_vector_(all_results, std::move(results));
    }

    auto filtered_results = NeuralNetwork::DetectionPostProcessor::select_best_per_expected(
        all_results,
        threshold_policy.expected_thresholds,
        [](const CandidateResult& result) { return static_cast<int>(result.cls_index); },
        &CandidateResult::score);
    finalize(std::move(all_results), std::move(filtered_results));

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model) << VAR(labels)
             << VAR(param_.expected) << VAR(param_.thresholds);
}

bool NeuralNetworkDetector::detect(
    const std::vector<std::string>& labels,
    double score_floor,
    CandidateResults& results,
    std::string& error) const
{
    using namespace NeuralNetwork;

    try {
        PreprocessedImage preprocessed;
        if (!ImagePreprocessor::preprocess(image_with_roi(), package_->descriptor.preprocess, preprocessed, error)) {
            return false;
        }

        std::vector<Ort::Value> input_tensors;
        input_tensors.emplace_back(
            Ort::Value::CreateTensor<float>(
                *package_->memory_info,
                preprocessed.tensor.data(),
                preprocessed.tensor.size(),
                preprocessed.shape.data(),
                preprocessed.shape.size()));

        std::vector<const char*> input_names;
        input_names.reserve(package_->input_names.size());
        std::ranges::transform(package_->input_names, std::back_inserter(input_names), [](const std::string& name) {
            return name.c_str();
        });
        std::vector<const char*> output_names;
        output_names.reserve(package_->output_names.size());
        std::ranges::transform(package_->output_names, std::back_inserter(output_names), [](const std::string& name) {
            return name.c_str();
        });

        Ort::RunOptions run_options;
        auto output_tensors = package_->session->Run(
            run_options,
            input_names.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names.data(),
            output_names.size());
        if (output_tensors.size() != package_->output_names.size()) {
            error = "ONNX Runtime returned an unexpected output count";
            return false;
        }

        std::vector<TensorView> tensors;
        tensors.reserve(output_tensors.size());
        for (size_t index = 0; index < output_tensors.size(); ++index) {
            if (!output_tensors[index].IsTensor()) {
                error = "detector output is not a tensor";
                return false;
            }
            const auto info = output_tensors[index].GetTensorTypeAndShapeInfo();
            if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                error = "detector output is not FP32";
                return false;
            }
            tensors.emplace_back(
                TensorView {
                    .name = package_->output_names[index],
                    .data = { output_tensors[index].GetTensorData<float>(), info.GetElementCount() },
                    .shape = info.GetShape(),
                });
        }

        DecodedDetections decoded;
        if (!package_->adapter->decode(
                tensors,
                preprocessed.transform,
                DecodeOptions { .score_floor = static_cast<float>(score_floor) },
                decoded,
                error)) {
            return false;
        }
        if (!labels.empty() && decoded.class_count && *decoded.class_count != labels.size()) {
            error = "label count does not match the detector output class count";
            return false;
        }
        for (const auto& candidate : decoded.candidates) {
            if (!labels.empty() && static_cast<size_t>(candidate.class_id) >= labels.size()) {
                error = "detector class id is outside the available label table";
                return false;
            }
        }
        if (!DetectionPostProcessor::sanitize(decoded.candidates, image_with_roi().size(), error)) {
            return false;
        }

        DetectionPostProcessor::filter_by_score(decoded.candidates, score_floor);
        const NmsPolicy nms = param_.nms.value_or(package_->descriptor.nms);
        const float nms_threshold = static_cast<float>(param_.nms_threshold.value_or(package_->descriptor.nms_threshold));
        auto candidates = DetectionPostProcessor::nms(std::move(decoded.candidates), nms, nms_threshold);

        results.clear();
        results.reserve(candidates.size());
        for (const auto& candidate : candidates) {
            cv::Rect2f box = candidate.box;
            box.x += static_cast<float>(roi_.x);
            box.y += static_cast<float>(roi_.y);
            box &= cv::Rect2f { 0.0F, 0.0F, static_cast<float>(image_.cols), static_cast<float>(image_.rows) };
            if (box.empty()) {
                continue;
            }

            std::string label;
            if (!LabelParser::label_for(candidate.class_id, labels, label, error)) {
                return false;
            }
            results.emplace_back(
                CandidateResult {
                    .cls_index = static_cast<size_t>(candidate.class_id),
                    .label = std::move(label),
                    .box = box,
                    .score = candidate.score,
                });
        }

        if (debug_draw_) {
            ResultsVec debug_results;
            debug_results.reserve(results.size());
            std::ranges::transform(results, std::back_inserter(debug_results), [this](const CandidateResult& result) {
                return to_result(result);
            });
            handle_draw(draw_result(debug_results));
        }
        error.clear();
        return true;
    }
    catch (const Ort::Exception& exception) {
        error = std::string("ONNX Runtime inference error: ") + exception.what();
        return false;
    }
    catch (const std::exception& exception) {
        error = std::string("detector inference error: ") + exception.what();
        return false;
    }
}

void NeuralNetworkDetector::finalize(CandidateResults all_results, CandidateResults filtered_results)
{
    sort_(all_results);
    sort_(filtered_results);
    const auto best_index = pythonic_index(filtered_results.size(), param_.result_index);

    all_results_.clear();
    all_results_.reserve(all_results.size());
    for (auto& result : all_results) {
        all_results_.emplace_back(to_result(std::move(result)));
    }

    filtered_results_.clear();
    filtered_results_.reserve(filtered_results.size());
    for (auto& result : filtered_results) {
        filtered_results_.emplace_back(to_result(std::move(result)));
    }

    if (best_index) {
        best_result_ = filtered_results_.at(*best_index);
    }
}

NeuralNetworkDetector::Result NeuralNetworkDetector::to_result(CandidateResult result) const
{
    cv::Rect box = NeuralNetwork::DetectionPostProcessor::to_integer_box(result.box);
    box &= cv::Rect { 0, 0, image_.cols, image_.rows };
    return {
        .cls_index = result.cls_index,
        .label = std::move(result.label),
        .box = box,
        .score = result.score,
    };
}

cv::Mat NeuralNetworkDetector::draw_result(const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();

    for (const Result& res : results) {
        const cv::Rect& my_box = res.box;

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, my_box, color, 1);
        std::string flag = std::format(
            "{} {} {:.3f}: [{}, {}, {}, {}]",
            res.cls_index,
            res.label,
            res.score,
            my_box.x,
            my_box.y,
            my_box.width,
            my_box.height);
        cv::putText(image_draw, flag, cv::Point(my_box.x, my_box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    return image_draw;
}

void NeuralNetworkDetector::sort_(CandidateResults& results) const
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
    case ResultOrderBy::Expected:
        sort_by_expected_index_(results, param_.expected);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

MAA_VISION_NS_END
