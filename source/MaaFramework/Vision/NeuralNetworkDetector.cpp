#include "NeuralNetworkDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <limits>
#include <ranges>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "NeuralNetworkAdapter/ModelResolver.h"
#include "NeuralNetworkAdapter/NeuralNetworkAdapter.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

namespace
{

using NeuralNetworkAdapter::BoxDecodeSpec;
using NeuralNetworkAdapter::BoxFormat;
using NeuralNetworkAdapter::ColorOrder;
using NeuralNetworkAdapter::CoordinateSpace;
using NeuralNetworkAdapter::DecodedDetections;
using NeuralNetworkAdapter::IAdapter;
using NeuralNetworkAdapter::ImageLayout;
using NeuralNetworkAdapter::InputPipelineSpec;
using NeuralNetworkAdapter::ModelDescriptor;
using NeuralNetworkAdapter::ModelFacts;
using NeuralNetworkAdapter::OriginalSizeOrder;
using NeuralNetworkAdapter::PaddingPosition;
using NeuralNetworkAdapter::RawDetection;
using NeuralNetworkAdapter::ResizeInterpolation;
using NeuralNetworkAdapter::ResizeMode;
using NeuralNetworkAdapter::TensorElementType;
using NeuralNetworkAdapter::TensorSpec;
using NeuralNetworkAdapter::TensorView;

struct CoordinateTransform
{
    cv::Size source_size;
    cv::Size input_size;
    float scale_x = 1.0F;
    float scale_y = 1.0F;
    float padding_x = 0.0F;
    float padding_y = 0.0F;

    cv::Rect2f to_source(const cv::Rect2f& box) const
    {
        return {
            (box.x - padding_x) / scale_x,
            (box.y - padding_y) / scale_y,
            box.width / scale_x,
            box.height / scale_y,
        };
    }
};

struct PreprocessedImage
{
    std::vector<float> tensor;
    std::array<int64_t, 4> shape = { 1, 3, 0, 0 };
    CoordinateTransform transform;
};

struct DetectionCandidate
{
    cv::Rect2f box;
    float score = 0.0F;
    int class_id = -1;
};

struct ExpectedThreshold
{
    int class_id = -1;
    double threshold = 0.0;
};

struct ThresholdPolicy
{
    double score_floor = 0.0;
    std::vector<ExpectedThreshold> expected_thresholds;
};

std::optional<TensorElementType> tensor_element_type(ONNXTensorElementDataType type)
{
    if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        return TensorElementType::Float32;
    }
    if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16) {
        return TensorElementType::Float16;
    }
    if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
        return TensorElementType::Int64;
    }
    return std::nullopt;
}

cv::Size resolve_input_size(const cv::Mat& image, const InputPipelineSpec& spec, const std::optional<cv::Size>& pipeline_size)
{
    const size_t height_axis = spec.layout == ImageLayout::NCHW ? 2 : 1;
    const size_t width_axis = spec.layout == ImageLayout::NCHW ? 3 : 2;
    const int64_t static_width = spec.graph_shape[width_axis];
    const int64_t static_height = spec.graph_shape[height_axis];
    const int width = static_width > 0 ? static_cast<int>(static_width)
                                       : (pipeline_size ? pipeline_size->width : (spec.resize_size ? spec.resize_size->width : image.cols));
    const int height = static_height > 0
                           ? static_cast<int>(static_height)
                           : (pipeline_size ? pipeline_size->height : (spec.resize_size ? spec.resize_size->height : image.rows));
    return { width, height };
}

int cv_interpolation(ResizeInterpolation interpolation)
{
    switch (interpolation) {
    case ResizeInterpolation::Nearest:
        return cv::INTER_NEAREST;
    case ResizeInterpolation::Linear:
        return cv::INTER_LINEAR;
    case ResizeInterpolation::Cubic:
        return cv::INTER_CUBIC;
    case ResizeInterpolation::Area:
        return cv::INTER_AREA;
    }
    return cv::INTER_LINEAR;
}

bool preprocess(
    const cv::Mat& image,
    const InputPipelineSpec& spec,
    const std::optional<cv::Size>& pipeline_size,
    PreprocessedImage& output,
    std::string& error)
{
    if (image.empty() || image.type() != CV_8UC3) {
        error = "detector input image must be non-empty CV_8UC3";
        return false;
    }
    const cv::Size input_size = resolve_input_size(image, spec, pipeline_size);
    if (input_size.width <= 0 || input_size.height <= 0) {
        error = "detector input size must be positive";
        return false;
    }

    const bool letterbox = spec.resize_mode == ResizeMode::Letterbox;
    const int interpolation = cv_interpolation(spec.interpolation);
    cv::Mat resized;
    CoordinateTransform transform {
        .source_size = image.size(),
        .input_size = input_size,
    };
    if (!letterbox) {
        cv::resize(image, resized, input_size, 0.0, 0.0, interpolation);
        transform.scale_x = static_cast<float>(input_size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(input_size.height) / static_cast<float>(image.rows);
    }
    else {
        const float scale = std::min(
            static_cast<float>(input_size.width) / static_cast<float>(image.cols),
            static_cast<float>(input_size.height) / static_cast<float>(image.rows));
        const cv::Size resized_size {
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.cols) * scale))),
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.rows) * scale))),
        };
        cv::Mat scaled;
        cv::resize(image, scaled, resized_size, 0.0, 0.0, interpolation);
        const int left = spec.padding_position == PaddingPosition::Center ? (input_size.width - resized_size.width) / 2 : 0;
        const int top = spec.padding_position == PaddingPosition::Center ? (input_size.height - resized_size.height) / 2 : 0;
        const int right = input_size.width - resized_size.width - left;
        const int bottom = input_size.height - resized_size.height - top;
        cv::copyMakeBorder(
            scaled,
            resized,
            top,
            bottom,
            left,
            right,
            cv::BORDER_CONSTANT,
            cv::Scalar(spec.fill[0], spec.fill[1], spec.fill[2]));
        transform.scale_x = static_cast<float>(resized_size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(resized_size.height) / static_cast<float>(image.rows);
        transform.padding_x = static_cast<float>(left);
        transform.padding_y = static_cast<float>(top);
    }

    if (spec.color == ColorOrder::RGB) {
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    }
    cv::Mat float_image;
    resized.convertTo(float_image, CV_32FC3, spec.scale);
    std::array<cv::Mat, 3> channels;
    cv::split(float_image, channels);

    const size_t plane_size = static_cast<size_t>(input_size.area());
    for (size_t channel = 0; channel < channels.size(); ++channel) {
        channels[channel] = (channels[channel] - spec.mean[channel]) / spec.std[channel];
    }
    output.tensor.resize(plane_size * channels.size());
    if (spec.layout == ImageLayout::NCHW) {
        for (size_t channel = 0; channel < channels.size(); ++channel) {
            std::copy_n(
                channels[channel].ptr<float>(),
                plane_size,
                output.tensor.begin() + static_cast<std::ptrdiff_t>(channel * plane_size));
        }
        output.shape = { 1, 3, input_size.height, input_size.width };
    }
    else {
        cv::merge(channels, float_image);
        std::copy_n(float_image.ptr<float>(), output.tensor.size(), output.tensor.begin());
        output.shape = { 1, input_size.height, input_size.width, 3 };
    }
    output.transform = transform;
    error.clear();
    return true;
}

cv::Rect2f decode_box(const RawDetection& raw, const BoxDecodeSpec& spec, const CoordinateTransform& transform)
{
    std::array<float, 4> values = raw.box;
    if (spec.coordinates == CoordinateSpace::Normalized) {
        values[0] *= static_cast<float>(transform.input_size.width);
        values[1] *= static_cast<float>(transform.input_size.height);
        values[2] *= static_cast<float>(transform.input_size.width);
        values[3] *= static_cast<float>(transform.input_size.height);
    }
    cv::Rect2f input_box;
    if (spec.format == BoxFormat::CxCyWh) {
        input_box = { values[0] - values[2] / 2.0F, values[1] - values[3] / 2.0F, values[2], values[3] };
    }
    else {
        input_box = { values[0], values[1], values[2] - values[0], values[3] - values[1] };
    }
    return spec.coordinates == CoordinateSpace::Source ? input_box : transform.to_source(input_box);
}

cv::Rect truncate_box(const cv::Rect2f& box)
{
    return {
        static_cast<int>(box.x),
        static_cast<int>(box.y),
        static_cast<int>(box.width),
        static_cast<int>(box.height),
    };
}

int64_t area(const cv::Rect& box)
{
    return static_cast<int64_t>(box.width) * box.height;
}

float iou(const cv::Rect2f& lhs, const cv::Rect2f& rhs)
{
    const cv::Rect left = truncate_box(lhs);
    const cv::Rect right = truncate_box(rhs);
    const int64_t intersection = area(left & right);
    const int64_t union_area = area(left) + area(right) - intersection;
    return union_area > 0 ? static_cast<float>(intersection) / static_cast<float>(union_area) : 0.0F;
}

bool sanitize(std::vector<DetectionCandidate>& candidates, cv::Size roi_size, std::string& error)
{
    if (roi_size.width <= 0 || roi_size.height <= 0) {
        error = "ROI size must be positive";
        return false;
    }
    std::vector<DetectionCandidate> output;
    output.reserve(candidates.size());
    for (auto candidate : candidates) {
        const auto& box = candidate.box;
        if (!std::isfinite(box.x) || !std::isfinite(box.y) || !std::isfinite(box.width) || !std::isfinite(box.height)
            || !std::isfinite(candidate.score)) {
            error = "detection candidate contains a non-finite value";
            return false;
        }
        if (candidate.class_id < 0 || box.width < 0.0F || box.height < 0.0F) {
            error = candidate.class_id < 0 ? "detection candidate contains a negative class id"
                                           : "detection candidate contains a negative box size";
            return false;
        }
        constexpr float kScoreTolerance = 1.0e-5F;
        if (candidate.score < -kScoreTolerance || candidate.score > 1.0F + kScoreTolerance) {
            error = "detection candidate score is outside [0, 1]";
            return false;
        }
        candidate.score = std::clamp(candidate.score, 0.0F, 1.0F);

        constexpr double kIntMin = std::numeric_limits<int>::min();
        constexpr double kIntMax = std::numeric_limits<int>::max();
        if (box.x < kIntMin || box.x > kIntMax || box.y < kIntMin || box.y > kIntMax || box.width > kIntMax || box.height > kIntMax) {
            error = "detection candidate box exceeds the supported integer range";
            return false;
        }
        const cv::Rect integer = truncate_box(box);
        const int64_t left = std::clamp<int64_t>(integer.x, 0, roi_size.width);
        const int64_t top = std::clamp<int64_t>(integer.y, 0, roi_size.height);
        const int64_t right = std::clamp<int64_t>(static_cast<int64_t>(integer.x) + integer.width, 0, roi_size.width);
        const int64_t bottom = std::clamp<int64_t>(static_cast<int64_t>(integer.y) + integer.height, 0, roi_size.height);
        if (right <= left || bottom <= top) {
            continue;
        }
        candidate.box = {
            static_cast<float>(left),
            static_cast<float>(top),
            static_cast<float>(right - left),
            static_cast<float>(bottom - top),
        };
        output.emplace_back(std::move(candidate));
    }
    candidates = std::move(output);
    error.clear();
    return true;
}

std::vector<DetectionCandidate> apply_nms(std::vector<DetectionCandidate> candidates, NeuralNetwork::NmsPolicy policy, float threshold)
{
    std::ranges::stable_sort(candidates, std::greater { }, &DetectionCandidate::score);
    if (policy == NeuralNetwork::NmsPolicy::None) {
        return candidates;
    }

    std::vector<DetectionCandidate> output;
    std::vector<bool> suppressed(candidates.size(), false);
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (suppressed[i]) {
            continue;
        }
        output.emplace_back(candidates[i]);
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (suppressed[j]) {
                continue;
            }
            if (policy == NeuralNetwork::NmsPolicy::ClassAwareIoU) {
                if (candidates[i].class_id == candidates[j].class_id && iou(candidates[i].box, candidates[j].box) >= threshold) {
                    suppressed[j] = true;
                }
                continue;
            }
            const int64_t candidate_area = area(truncate_box(candidates[j].box));
            const int64_t intersection = area(truncate_box(candidates[i].box) & truncate_box(candidates[j].box));
            if (candidate_area > 0 && static_cast<double>(intersection) >= threshold * candidate_area) {
                suppressed[j] = true;
            }
        }
    }
    return output;
}

bool build_threshold_policy(
    const std::vector<int>& expected,
    const std::vector<double>& thresholds,
    ThresholdPolicy& policy,
    std::string& error)
{
    policy = { };
    if (std::ranges::any_of(thresholds, [](double value) { return !std::isfinite(value) || value < 0.0 || value > 1.0; })) {
        error = "threshold values must be in [0, 1]";
        return false;
    }
    if (expected.empty()) {
        policy.score_floor = thresholds.empty() ? NeuralNetworkDetectorParam::kDefaultThreshold : thresholds.front();
        error.clear();
        return true;
    }
    if (expected.size() != thresholds.size()) {
        error = "expected and threshold counts do not match";
        return false;
    }
    for (size_t index = 0; index < expected.size(); ++index) {
        if (expected[index] < 0) {
            error = "expected class ids must be non-negative";
            return false;
        }
        if (std::ranges::find(policy.expected_thresholds, expected[index], &ExpectedThreshold::class_id)
            == policy.expected_thresholds.end()) {
            policy.expected_thresholds.emplace_back(ExpectedThreshold { .class_id = expected[index], .threshold = thresholds[index] });
        }
    }
    policy.score_floor = std::ranges::min(policy.expected_thresholds, { }, &ExpectedThreshold::threshold).threshold;
    error.clear();
    return true;
}

bool resolve_labels(
    const std::vector<std::string>& pipeline,
    const std::vector<std::string>& descriptor,
    const std::vector<std::string>& metadata,
    std::vector<std::string>& output,
    std::string& error)
{
    const auto& selected = !pipeline.empty() ? pipeline : (!metadata.empty() ? metadata : descriptor);
    if (selected.empty()) {
        output.clear();
        error.clear();
        return true;
    }
    if (!NeuralNetworkAdapter::validate_labels(selected, error)) {
        return false;
    }
    output = selected;
    error.clear();
    return true;
}

std::string label_for(int class_id, const std::vector<std::string>& labels)
{
    return labels.empty() ? std::format("Unknown_{}", class_id) : labels[static_cast<size_t>(class_id)];
}

} // namespace

struct NeuralNetworkDetector::Model
{
    std::filesystem::path onnx_path;
    std::shared_ptr<Ort::Session> session;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    ModelDescriptor descriptor;
    std::shared_ptr<const IAdapter> adapter;
    std::vector<std::string> metadata_labels;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<TensorElementType> output_types;
};

NeuralNetworkDetector::ModelLoadResult
    NeuralNetworkDetector::load_model(const std::filesystem::path& onnx_path, std::shared_ptr<Ort::Session> session)
{
    if (!session) {
        return { .error = "detector ONNX Session is null" };
    }

    try {
        std::filesystem::path descriptor_path = onnx_path;
        descriptor_path.replace_extension(".json");
        const bool has_descriptor = std::filesystem::exists(descriptor_path);
        std::optional<json::value> descriptor_json;
        if (has_descriptor) {
            descriptor_json = json::open(descriptor_path, true, false);
            if (!descriptor_json) {
                return { .error = "failed to parse model descriptor: " + descriptor_path.string() };
            }
        }

        Ort::AllocatorWithDefaultOptions allocator;
        ModelFacts facts;
        facts.inputs.reserve(session->GetInputCount());
        for (size_t index = 0; index < session->GetInputCount(); ++index) {
            const auto info = session->GetInputTypeInfo(index).GetTensorTypeAndShapeInfo();
            const auto element_type = tensor_element_type(info.GetElementType());
            if (!element_type) {
                return { .error = "detector input uses an unsupported tensor element type" };
            }
            facts.inputs.emplace_back(
                TensorSpec {
                    .name = session->GetInputNameAllocated(index, allocator).get(),
                    .element_type = *element_type,
                    .shape = info.GetShape(),
                });
        }

        facts.outputs.reserve(session->GetOutputCount());
        for (size_t index = 0; index < session->GetOutputCount(); ++index) {
            const auto info = session->GetOutputTypeInfo(index).GetTensorTypeAndShapeInfo();
            const auto element_type = tensor_element_type(info.GetElementType());
            if (!element_type) {
                return { .error = "detector output uses an unsupported tensor element type" };
            }
            facts.outputs.emplace_back(
                TensorSpec {
                    .name = session->GetOutputNameAllocated(index, allocator).get(),
                    .element_type = *element_type,
                    .shape = info.GetShape(),
                });
        }

        const Ort::ModelMetadata metadata = session->GetModelMetadata();
        for (auto& key : metadata.GetCustomMetadataMapKeysAllocated(allocator)) {
            auto value = metadata.LookupCustomMetadataMapAllocated(key.get(), allocator);
            if (value) {
                facts.metadata.emplace(key.get(), value.get());
            }
        }

        NeuralNetworkAdapter::ResolvedModel resolved;
        std::string error;
        if (!NeuralNetworkAdapter::resolve_model(facts, descriptor_json ? &*descriptor_json : nullptr, resolved, error)) {
            return { .error = has_descriptor ? "invalid model descriptor " + descriptor_path.string() + ": " + error
                                             : "failed to resolve detector model protocol: " + error };
        }

        auto adapter = NeuralNetworkAdapter::create(resolved.descriptor.output.adapter);
        if (!adapter) {
            return { .error = "failed to create detector output adapter" };
        }

        std::vector<std::string> output_names;
        std::vector<TensorElementType> output_types;
        output_names.reserve(resolved.outputs.size());
        output_types.reserve(resolved.outputs.size());
        for (const auto& output : resolved.outputs) {
            output_names.emplace_back(output.name);
            output_types.emplace_back(output.element_type);
        }

        auto model = std::make_shared<Model>();
        model->onnx_path = onnx_path;
        model->session = std::move(session);
        model->descriptor = std::move(resolved.descriptor);
        model->adapter = std::shared_ptr<const IAdapter>(std::move(adapter));
        model->metadata_labels = std::move(resolved.metadata_labels);
        model->input_names = std::move(resolved.input_names);
        model->output_names = std::move(output_names);
        model->output_types = std::move(output_types);
        return { .model = std::move(model) };
    }
    catch (const Ort::Exception& exception) {
        return { .error = std::string("ONNX Runtime error: ") + exception.what() };
    }
    catch (const std::exception& exception) {
        return { .error = std::string("failed to load detector model: ") + exception.what() };
    }
}

NeuralNetworkDetector::NeuralNetworkDetector(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    NeuralNetworkDetectorParam param,
    std::shared_ptr<const Model> model,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , model_(std::move(model))
{
    analyze();
}

void NeuralNetworkDetector::analyze()
{
    LogFunc << name_;
    if (!model_ || !model_->session || !model_->adapter) {
        error_ = "detector model is not loaded";
        LogError << error_;
        return;
    }

    const auto start_time = std::chrono::steady_clock::now();
    std::vector<std::string> labels;
    if (!resolve_labels(param_.labels, model_->descriptor.labels, model_->metadata_labels, labels, error_)) {
        LogError << name_ << VAR(error_);
        return;
    }
    ThresholdPolicy threshold_policy;
    if (!build_threshold_policy(param_.expected, param_.thresholds, threshold_policy, error_)) {
        LogError << name_ << VAR(error_);
        return;
    }
    const double decode_score_floor = threshold_policy.score_floor;

    CandidateResults all_results;
    while (next_roi()) {
        CandidateResults results;
        if (!detect(labels, decode_score_floor, results, error_)) {
            all_results_.clear();
            filtered_results_.clear();
            best_result_.reset();
            LogError << name_ << VAR(error_);
            return;
        }
        merge_vector_(all_results, std::move(results));
    }

    CandidateResults filtered_results;
    if (threshold_policy.expected_thresholds.empty()) {
        std::ranges::copy_if(all_results, std::back_inserter(filtered_results), [&](const auto& result) {
            return result.score >= threshold_policy.score_floor;
        });
    }
    else {
        filtered_results.reserve(threshold_policy.expected_thresholds.size());
        for (const auto& expected : threshold_policy.expected_thresholds) {
            const CandidateResult* best = nullptr;
            for (const auto& result : all_results) {
                if (static_cast<int>(result.cls_index) != expected.class_id || result.score < expected.threshold) {
                    continue;
                }
                if (!best || result.score > best->score) {
                    best = &result;
                }
            }
            if (best) {
                filtered_results.emplace_back(*best);
            }
        }
    }
    finalize(std::move(all_results), std::move(filtered_results));

    const auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model) << VAR(labels)
             << VAR(param_.expected) << VAR(param_.thresholds);
}

bool NeuralNetworkDetector::detect(
    const std::vector<std::string>& labels,
    double score_floor,
    CandidateResults& results,
    std::string& error) const
{
    try {
        PreprocessedImage preprocessed;
        if (!preprocess(image_with_roi(), model_->descriptor.input, param_.input_size, preprocessed, error)) {
            return false;
        }

        std::array<int64_t, 2> original_size { };
        std::array<int64_t, 2> original_size_shape = { 1, 2 };
        std::vector<Ort::Float16_t> float16_input;
        std::vector<Ort::Value> input_tensors;
        input_tensors.reserve(model_->input_names.size());
        if (model_->descriptor.input.element_type == TensorElementType::Float16) {
            float16_input.reserve(preprocessed.tensor.size());
            std::ranges::transform(preprocessed.tensor, std::back_inserter(float16_input), [](float value) {
                return Ort::Float16_t(value);
            });
            input_tensors.emplace_back(
                Ort::Value::CreateTensor<Ort::Float16_t>(
                    model_->memory_info,
                    float16_input.data(),
                    float16_input.size(),
                    preprocessed.shape.data(),
                    preprocessed.shape.size()));
        }
        else {
            input_tensors.emplace_back(
                Ort::Value::CreateTensor<float>(
                    model_->memory_info,
                    preprocessed.tensor.data(),
                    preprocessed.tensor.size(),
                    preprocessed.shape.data(),
                    preprocessed.shape.size()));
        }
        if (const auto& spec = model_->descriptor.input.original_size) {
            original_size = spec->order == OriginalSizeOrder::HW ? std::array<int64_t, 2> { image_with_roi().rows, image_with_roi().cols }
                                                                 : std::array<int64_t, 2> { image_with_roi().cols, image_with_roi().rows };
            input_tensors.emplace_back(
                Ort::Value::CreateTensor<int64_t>(
                    model_->memory_info,
                    original_size.data(),
                    original_size.size(),
                    original_size_shape.data(),
                    original_size_shape.size()));
        }

        std::vector<const char*> input_names;
        std::ranges::transform(model_->input_names, std::back_inserter(input_names), [](const std::string& name) { return name.c_str(); });
        std::vector<const char*> output_names;
        std::ranges::transform(model_->output_names, std::back_inserter(output_names), [](const std::string& name) {
            return name.c_str();
        });

        Ort::RunOptions run_options;
        auto output_tensors = model_->session->Run(
            run_options,
            input_names.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names.data(),
            output_names.size());
        if (output_tensors.size() != model_->output_names.size()) {
            error = "ONNX Runtime returned an unexpected output count";
            return false;
        }

        std::vector<TensorView> tensors;
        tensors.reserve(output_tensors.size());
        std::vector<std::vector<float>> float16_outputs(output_tensors.size());
        for (size_t index = 0; index < output_tensors.size(); ++index) {
            if (!output_tensors[index].IsTensor()) {
                error = "detector output is not a tensor";
                return false;
            }
            const auto info = output_tensors[index].GetTensorTypeAndShapeInfo();
            const auto element_type = tensor_element_type(info.GetElementType());
            if (!element_type || *element_type != model_->output_types[index]) {
                error = "detector output tensor type changed at runtime";
                return false;
            }
            TensorView view {
                .name = model_->output_names[index],
                .element_type = *element_type,
                .shape = info.GetShape(),
            };
            if (*element_type == TensorElementType::Float32) {
                view.float_data = { output_tensors[index].GetTensorData<float>(), info.GetElementCount() };
            }
            else if (*element_type == TensorElementType::Float16) {
                const auto* data = output_tensors[index].GetTensorData<Ort::Float16_t>();
                auto& converted = float16_outputs[index];
                converted.reserve(info.GetElementCount());
                for (size_t element = 0; element < info.GetElementCount(); ++element) {
                    converted.emplace_back(data[element].ToFloat());
                }
                view.float_data = converted;
            }
            else if (*element_type == TensorElementType::Int64) {
                view.int64_data = { output_tensors[index].GetTensorData<int64_t>(), info.GetElementCount() };
            }
            else {
                error = "detector output uses an unsupported runtime tensor type";
                return false;
            }
            tensors.emplace_back(std::move(view));
        }

        DecodedDetections decoded;
        if (!model_->adapter->decode(
                tensors,
                NeuralNetworkAdapter::DecodeOptions {
                    .score_floor = static_cast<float>(score_floor),
                    .multi_label = param_.multi_label,
                },
                decoded,
                error)) {
            return false;
        }

        std::vector<DetectionCandidate> candidates;
        candidates.reserve(decoded.candidates.size());
        for (const auto& raw : decoded.candidates) {
            candidates.emplace_back(
                DetectionCandidate {
                    .box = decode_box(raw, model_->descriptor.output.box_decode, preprocessed.transform),
                    .score = raw.score,
                    .class_id = raw.class_id,
                });
        }
        if (!labels.empty() && decoded.class_count && *decoded.class_count != labels.size()) {
            error = "label count does not match the detector output class count";
            return false;
        }
        for (const auto& candidate : candidates) {
            if (!labels.empty() && static_cast<size_t>(candidate.class_id) >= labels.size()) {
                error = "detector class id is outside the available label table";
                return false;
            }
        }
        if (!sanitize(candidates, image_with_roi().size(), error)) {
            return false;
        }
        std::erase_if(candidates, [score_floor](const auto& candidate) { return candidate.score < score_floor; });

        candidates = apply_nms(std::move(candidates), model_->descriptor.nms, model_->descriptor.nms_threshold);

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
            results.emplace_back(
                CandidateResult {
                    .cls_index = static_cast<size_t>(candidate.class_id),
                    .label = label_for(candidate.class_id, labels),
                    .box = box,
                    .score = candidate.score,
                });
        }

        if (debug_draw_) {
            ResultsVec debug_results;
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
    for (auto& result : all_results) {
        all_results_.emplace_back(to_result(std::move(result)));
    }
    filtered_results_.clear();
    for (auto& result : filtered_results) {
        filtered_results_.emplace_back(to_result(std::move(result)));
    }
    if (best_index) {
        best_result_ = filtered_results_.at(*best_index);
    }
}

NeuralNetworkDetector::Result NeuralNetworkDetector::to_result(CandidateResult result) const
{
    cv::Rect box = truncate_box(result.box);
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
    for (const Result& result : results) {
        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, result.box, color, 1);
        const std::string flag = std::format(
            "{} {} {:.3f}: [{}, {}, {}, {}]",
            result.cls_index,
            result.label,
            result.score,
            result.box.x,
            result.box.y,
            result.box.width,
            result.box.height);
        cv::putText(image_draw, flag, cv::Point(result.box.x, result.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
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
