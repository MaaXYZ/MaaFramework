#include "NeuralNetworkDetector.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <format>
#include <functional>
#include <limits>
#include <ranges>
#include <unordered_set>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "NeuralNetworkAdapter/NeuralNetworkAdapter.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

namespace
{

using NeuralNetworkAdapter::AdapterOptions;
using NeuralNetworkAdapter::DecodedDetections;
using NeuralNetworkAdapter::DetrActivation;
using NeuralNetworkAdapter::DetrPackedLayout;
using NeuralNetworkAdapter::IAdapter;
using NeuralNetworkAdapter::OutputProtocol;
using NeuralNetworkAdapter::RawDetection;
using NeuralNetworkAdapter::TensorElementType;
using NeuralNetworkAdapter::TensorSpec;
using NeuralNetworkAdapter::TensorView;
using NeuralNetworkAdapter::YoloDenseLayout;

enum class InputProtocol
{
    UltralyticsLetterbox,
    DirectResizeDivide255,
    DirectResizeImageNet,
};

enum class OriginalSizeOrder
{
    HW,
    WH,
};

enum class BoxEncoding
{
    InputCxcywh,
    InputXyxy,
    NormalizedCxcywh,
    SourceXyxy,
};

struct OriginalSizeInput
{
    std::string name;
    OriginalSizeOrder order = OriginalSizeOrder::HW;
};

struct InputProtocolSpec
{
    InputProtocol type = InputProtocol::DirectResizeDivide255;
    std::string name;
    cv::Size size;
    std::optional<OriginalSizeInput> original_size;
    bool legacy = false;
};

struct OutputProtocolSpec
{
    AdapterOptions adapter;
    std::vector<std::string> output_names;
    BoxEncoding box_encoding = BoxEncoding::InputCxcywh;
};

struct ModelDescriptor
{
    InputProtocolSpec input;
    OutputProtocolSpec output;
    std::vector<std::string> labels;
    NeuralNetwork::NmsPolicy nms = NeuralNetwork::NmsPolicy::ClassAwareIoU;
    float nms_threshold = 0.7F;
};

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

bool check_allowed_fields(
    const json::object& object,
    const std::unordered_set<std::string_view>& allowed,
    std::string_view context,
    std::string& error)
{
    for (const auto& [key, value] : object) {
        (void)value;
        if (!allowed.contains(key)) {
            error = "unknown " + std::string(context) + " field: " + key;
            return false;
        }
    }
    return true;
}

bool parse_positive_size(const json::value& value, cv::Size& output, std::string& error)
{
    if (!value.is_array() || value.as_array().size() != 2) {
        error = "input_protocol.size must be [width, height]";
        return false;
    }
    const auto& width_value = value.as_array()[0];
    const auto& height_value = value.as_array()[1];
    if (!width_value.is_number() || !height_value.is_number()) {
        error = "input_protocol.size must contain positive integers";
        return false;
    }
    const double width = width_value.as_double();
    const double height = height_value.as_double();
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0 || std::trunc(width) != width
        || std::trunc(height) != height || width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()) {
        error = "input_protocol.size must contain positive integers";
        return false;
    }
    output = { static_cast<int>(width), static_cast<int>(height) };
    return true;
}

bool parse_labels(const json::value& value, std::vector<std::string>& labels, std::string& error)
{
    std::vector<std::string> parsed;
    if (value.is_array()) {
        for (const auto& item : value.as_array()) {
            if (!item.is_string()) {
                error = "labels array must contain only strings";
                return false;
            }
            parsed.emplace_back(item.as_string());
        }
    }
    else if (value.is_object()) {
        std::vector<std::pair<size_t, std::string>> entries;
        entries.reserve(value.as_object().size());
        for (const auto& [key_text, item] : value.as_object()) {
            size_t key = 0;
            const auto [end, conversion_error] = std::from_chars(key_text.data(), key_text.data() + key_text.size(), key);
            if (conversion_error != std::errc() || end != key_text.data() + key_text.size() || !item.is_string()) {
                error = "labels object must map decimal class ids to strings";
                return false;
            }
            entries.emplace_back(key, item.as_string());
        }
        std::ranges::sort(entries, { }, &std::pair<size_t, std::string>::first);
        for (size_t index = 0; index < entries.size(); ++index) {
            if (entries[index].first != index) {
                error = "label class ids must be contiguous from zero";
                return false;
            }
            parsed.emplace_back(std::move(entries[index].second));
        }
    }
    else {
        error = "labels must be an array or object";
        return false;
    }

    if (parsed.empty()) {
        error = "labels must not be empty";
        return false;
    }
    std::unordered_set<std::string> unique;
    unique.reserve(parsed.size());
    for (const auto& label : parsed) {
        if (label.empty()) {
            error = "labels must not contain empty strings";
            return false;
        }
        if (!unique.emplace(label).second) {
            error = "labels must be unique";
            return false;
        }
    }
    labels = std::move(parsed);
    error.clear();
    return true;
}

void skip_spaces(std::string_view text, size_t& position)
{
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
}

bool parse_python_labels(std::string_view text, std::vector<std::string>& labels, std::string& error)
{
    size_t position = 0;
    skip_spaces(text, position);
    if (position == text.size() || text[position++] != '{') {
        error = "metadata labels are not a supported JSON value or Python dictionary";
        return false;
    }

    std::vector<std::pair<size_t, std::string>> entries;
    for (;;) {
        skip_spaces(text, position);
        if (position < text.size() && text[position] == '}') {
            ++position;
            break;
        }

        const size_t key_begin = position;
        while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]))) {
            ++position;
        }
        size_t key = 0;
        const auto [key_end, key_error] = std::from_chars(text.data() + key_begin, text.data() + position, key);
        if (key_begin == position || key_error != std::errc() || key_end != text.data() + position) {
            error = "metadata label keys must be non-negative decimal class ids";
            return false;
        }

        skip_spaces(text, position);
        if (position == text.size() || text[position++] != ':') {
            error = "invalid metadata labels dictionary";
            return false;
        }
        skip_spaces(text, position);
        if (position == text.size() || (text[position] != '\'' && text[position] != '"')) {
            error = "metadata labels must be strings";
            return false;
        }

        const char quote = text[position++];
        std::string label;
        bool closed = false;
        while (position < text.size()) {
            const char current = text[position++];
            if (current == quote) {
                closed = true;
                break;
            }
            if (current == '\\') {
                if (position == text.size()) {
                    error = "invalid escape in metadata label";
                    return false;
                }
                label.push_back(text[position++]);
                continue;
            }
            label.push_back(current);
        }
        if (!closed) {
            error = "unterminated metadata label";
            return false;
        }
        entries.emplace_back(key, std::move(label));

        skip_spaces(text, position);
        if (position < text.size() && text[position] == ',') {
            ++position;
            continue;
        }
        if (position < text.size() && text[position] == '}') {
            ++position;
            break;
        }
        error = "invalid metadata labels dictionary";
        return false;
    }

    skip_spaces(text, position);
    if (position != text.size()) {
        error = "unexpected characters after metadata labels dictionary";
        return false;
    }

    json::object object;
    for (auto& [index, label] : entries) {
        object.emplace(std::to_string(index), std::move(label));
    }
    return parse_labels(json::value(std::move(object)), labels, error);
}

bool parse_metadata_labels(std::string_view text, std::vector<std::string>& labels, std::string& error)
{
    if (auto value = json::parse(text)) {
        return parse_labels(*value, labels, error);
    }
    return parse_python_labels(text, labels, error);
}

std::vector<std::string> read_metadata_labels(const Ort::Session& session)
{
    Ort::AllocatorWithDefaultOptions allocator;
    const Ort::ModelMetadata metadata = session.GetModelMetadata();
    constexpr std::array<std::string_view, 4> kKeys = { "names", "name", "labels", "class_names" };
    for (const auto key : kKeys) {
        auto raw = metadata.LookupCustomMetadataMapAllocated(key.data(), allocator);
        if (!raw) {
            continue;
        }
        std::vector<std::string> labels;
        std::string error;
        if (parse_metadata_labels(raw.get(), labels, error)) {
            return labels;
        }
        LogWarn << "Failed to parse ONNX label metadata" << VAR(key) << VAR(error);
    }
    return { };
}

bool parse_input_protocol(const json::value& value, InputProtocolSpec& output, std::string& error)
{
    if (!value.is_object()) {
        error = "input_protocol must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowed = { "type", "input_name", "size", "original_size" };
    if (!check_allowed_fields(value.as_object(), kAllowed, "input_protocol", error)) {
        return false;
    }

    const auto type = value.find<std::string>("type");
    const auto name = value.find<std::string>("input_name");
    if (!type || !name || name->empty()) {
        error = "input_protocol requires non-empty type and input_name strings";
        return false;
    }
    if (*type == "UltralyticsLetterbox") {
        output.type = InputProtocol::UltralyticsLetterbox;
    }
    else if (*type == "DirectResizeDivide255") {
        output.type = InputProtocol::DirectResizeDivide255;
    }
    else if (*type == "DirectResizeImageNet") {
        output.type = InputProtocol::DirectResizeImageNet;
    }
    else {
        error = "input_protocol.type is not supported: " + *type;
        return false;
    }
    output.name = *name;

    if (auto size = value.find("size"); size && !parse_positive_size(*size, output.size, error)) {
        return false;
    }
    if (auto original_size = value.find("original_size")) {
        if (!original_size->is_object()) {
            error = "input_protocol.original_size must be an object";
            return false;
        }
        static const std::unordered_set<std::string_view> kOriginalAllowed = { "input_name", "order" };
        if (!check_allowed_fields(original_size->as_object(), kOriginalAllowed, "original_size", error)) {
            return false;
        }
        const auto original_name = original_size->find<std::string>("input_name");
        const auto order = original_size->find<std::string>("order");
        if (!original_name || original_name->empty() || !order) {
            error = "original_size requires non-empty input_name and order strings";
            return false;
        }
        OriginalSizeInput parsed { .name = *original_name };
        if (*order == "HW") {
            parsed.order = OriginalSizeOrder::HW;
        }
        else if (*order == "WH") {
            parsed.order = OriginalSizeOrder::WH;
        }
        else {
            error = "original_size.order must be HW or WH";
            return false;
        }
        if (parsed.name == output.name) {
            error = "image and original_size input names must differ";
            return false;
        }
        output.original_size = std::move(parsed);
    }
    error.clear();
    return true;
}

bool parse_optional_output_name(const json::value& value, std::vector<std::string>& names, std::string& error)
{
    if (auto name = value.find<std::string>("output_name")) {
        if (name->empty()) {
            error = "output_name must not be empty";
            return false;
        }
        names = { *name };
    }
    else if (value.exists("output_name")) {
        error = "output_name must be a string";
        return false;
    }
    return true;
}

bool parse_output_protocol(const json::value& value, OutputProtocolSpec& output, std::string& error)
{
    if (!value.is_object()) {
        error = "output_protocol must be an object";
        return false;
    }
    const auto type = value.find<std::string>("type");
    if (!type || type->empty()) {
        error = "output_protocol.type must be a non-empty string";
        return false;
    }

    if (*type == "YoloDense") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "layout", "output_name" };
        if (!check_allowed_fields(value.as_object(), kAllowed, "YoloDense", error)) {
            return false;
        }
        const auto layout = value.find<std::string>("layout");
        if (!layout) {
            error = "YoloDense.layout is required";
            return false;
        }
        output.adapter.protocol = OutputProtocol::YoloDense;
        output.box_encoding = BoxEncoding::InputCxcywh;
        if (*layout == "ChannelsFirst") {
            output.adapter.yolo_dense_layout = YoloDenseLayout::ChannelsFirst;
        }
        else if (*layout == "ChannelsLast") {
            output.adapter.yolo_dense_layout = YoloDenseLayout::ChannelsLast;
        }
        else {
            error = "YoloDense.layout must be ChannelsFirst or ChannelsLast";
            return false;
        }
        return parse_optional_output_name(value, output.output_names, error);
    }

    if (*type == "YoloEndToEnd") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "output_name" };
        if (!check_allowed_fields(value.as_object(), kAllowed, "YoloEndToEnd", error)) {
            return false;
        }
        output.adapter.protocol = OutputProtocol::YoloEndToEnd;
        output.box_encoding = BoxEncoding::InputXyxy;
        return parse_optional_output_name(value, output.output_names, error);
    }

    if (*type == "DetrQueries") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "boxes", "logits", "activation", "top_k" };
        if (!check_allowed_fields(value.as_object(), kAllowed, "DetrQueries", error)) {
            return false;
        }
        const auto boxes = value.find<std::string>("boxes");
        const auto logits = value.find<std::string>("logits");
        const auto activation = value.find<std::string>("activation");
        if (!boxes || boxes->empty() || !logits || logits->empty() || *boxes == *logits || !activation) {
            error = "DetrQueries requires distinct boxes and logits names and an activation";
            return false;
        }
        output.adapter.protocol = OutputProtocol::DetrQueries;
        output.box_encoding = BoxEncoding::NormalizedCxcywh;
        output.output_names = { *boxes, *logits };
        if (*activation == "Sigmoid") {
            output.adapter.detr_activation = DetrActivation::Sigmoid;
        }
        else if (*activation == "SoftmaxWithNoObject") {
            output.adapter.detr_activation = DetrActivation::SoftmaxWithNoObject;
        }
        else {
            error = "DetrQueries.activation must be Sigmoid or SoftmaxWithNoObject";
            return false;
        }
        if (auto top_k = value.find<double>("top_k")) {
            if (!std::isfinite(*top_k) || *top_k <= 0.0 || std::trunc(*top_k) != *top_k
                || *top_k > static_cast<double>(std::numeric_limits<size_t>::max())) {
                error = "DetrQueries.top_k must be a positive integer";
                return false;
            }
            output.adapter.detr_top_k = static_cast<size_t>(*top_k);
        }
        else if (value.exists("top_k")) {
            error = "DetrQueries.top_k must be a positive integer";
            return false;
        }
        error.clear();
        return true;
    }

    if (*type == "DetrPacked") {
        const auto layout = value.find<std::string>("layout");
        if (!layout) {
            error = "DetrPacked.layout is required";
            return false;
        }
        output.adapter.protocol = OutputProtocol::DetrPacked;
        if (*layout == "Interleaved") {
            static const std::unordered_set<std::string_view> kAllowed = { "type", "layout", "output_name" };
            if (!check_allowed_fields(value.as_object(), kAllowed, "DetrPacked Interleaved", error)) {
                return false;
            }
            output.adapter.detr_packed_layout = DetrPackedLayout::Interleaved;
            output.box_encoding = BoxEncoding::NormalizedCxcywh;
            return parse_optional_output_name(value, output.output_names, error);
        }
        if (*layout == "Split") {
            static const std::unordered_set<std::string_view> kAllowed = { "type", "layout", "labels", "boxes", "scores" };
            if (!check_allowed_fields(value.as_object(), kAllowed, "DetrPacked Split", error)) {
                return false;
            }
            const auto labels = value.find<std::string>("labels");
            const auto boxes = value.find<std::string>("boxes");
            const auto scores = value.find<std::string>("scores");
            if (!labels || labels->empty() || !boxes || boxes->empty() || !scores || scores->empty()) {
                error = "DetrPacked Split requires labels, boxes, and scores output names";
                return false;
            }
            std::unordered_set<std::string> unique = { *labels, *boxes, *scores };
            if (unique.size() != 3) {
                error = "DetrPacked Split output names must be distinct";
                return false;
            }
            output.adapter.detr_packed_layout = DetrPackedLayout::Split;
            output.box_encoding = BoxEncoding::SourceXyxy;
            output.output_names = { *labels, *boxes, *scores };
            error.clear();
            return true;
        }
        error = "DetrPacked.layout must be Interleaved or Split";
        return false;
    }

    error = "output_protocol.type is not supported: " + *type;
    return false;
}

bool parse_nms(std::string_view value, NeuralNetwork::NmsPolicy& output)
{
    if (value == "None") {
        output = NeuralNetwork::NmsPolicy::None;
        return true;
    }
    if (value == "ClassAwareIoU") {
        output = NeuralNetwork::NmsPolicy::ClassAwareIoU;
        return true;
    }
    if (value == "CandidateCoverage") {
        output = NeuralNetwork::NmsPolicy::CandidateCoverage;
        return true;
    }
    return false;
}

NeuralNetwork::NmsPolicy default_nms(OutputProtocol protocol)
{
    return protocol == OutputProtocol::YoloDense ? NeuralNetwork::NmsPolicy::ClassAwareIoU : NeuralNetwork::NmsPolicy::None;
}

bool parse_descriptor(const json::value& value, ModelDescriptor& output, std::string& error)
{
    if (!value.is_object()) {
        error = "model descriptor must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowed = {
        "$schema", "input_protocol", "output_protocol", "labels", "producer", "nms", "nms_threshold",
    };
    if (!check_allowed_fields(value.as_object(), kAllowed, "model descriptor", error)) {
        return false;
    }
    if (value.exists("$schema") && !value.find<std::string>("$schema")) {
        error = "$schema must be a string";
        return false;
    }
    if (auto producer = value.find("producer"); producer && !producer->is_object()) {
        error = "producer must be an object";
        return false;
    }
    const auto input = value.find("input_protocol");
    const auto output_protocol = value.find("output_protocol");
    if (!input || !output_protocol) {
        error = "input_protocol and output_protocol are required";
        return false;
    }

    ModelDescriptor parsed;
    if (!parse_input_protocol(*input, parsed.input, error) || !parse_output_protocol(*output_protocol, parsed.output, error)) {
        return false;
    }
    const bool split =
        parsed.output.adapter.protocol == OutputProtocol::DetrPacked && parsed.output.adapter.detr_packed_layout == DetrPackedLayout::Split;
    if (split != parsed.input.original_size.has_value()) {
        error = split ? "DetrPacked Split requires input_protocol.original_size"
                      : "input_protocol.original_size is only allowed for DetrPacked Split";
        return false;
    }

    parsed.nms = default_nms(parsed.output.adapter.protocol);
    if (auto labels = value.find("labels"); labels && !parse_labels(*labels, parsed.labels, error)) {
        return false;
    }
    if (auto nms = value.find<std::string>("nms")) {
        if (!parse_nms(*nms, parsed.nms)) {
            error = "nms is not a supported policy";
            return false;
        }
    }
    else if (value.exists("nms")) {
        error = "nms must be a string";
        return false;
    }
    if (auto threshold = value.find<double>("nms_threshold")) {
        if (!std::isfinite(*threshold) || *threshold < 0.0 || *threshold > 1.0) {
            error = "nms_threshold must be in [0, 1]";
            return false;
        }
        parsed.nms_threshold = static_cast<float>(*threshold);
    }
    else if (value.exists("nms_threshold")) {
        error = "nms_threshold must be a number";
        return false;
    }

    output = std::move(parsed);
    error.clear();
    return true;
}

ModelDescriptor legacy_descriptor()
{
    ModelDescriptor descriptor;
    descriptor.input.type = InputProtocol::DirectResizeDivide255;
    descriptor.input.legacy = true;
    descriptor.output.adapter.protocol = OutputProtocol::YoloDense;
    descriptor.output.adapter.yolo_dense_layout = YoloDenseLayout::ChannelsFirst;
    descriptor.output.adapter.yolo_dense_multi_label = true;
    descriptor.output.box_encoding = BoxEncoding::InputCxcywh;
    descriptor.nms = NeuralNetwork::NmsPolicy::CandidateCoverage;
    descriptor.nms_threshold = 0.7F;
    return descriptor;
}

std::optional<TensorElementType> tensor_element_type(ONNXTensorElementDataType type)
{
    if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        return TensorElementType::Float32;
    }
    if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
        return TensorElementType::Int64;
    }
    return std::nullopt;
}

const TensorSpec* find_tensor(std::span<const TensorSpec> tensors, std::string_view name)
{
    const auto iterator = std::ranges::find(tensors, name, &TensorSpec::name);
    return iterator == tensors.end() ? nullptr : &*iterator;
}

bool validate_image_input(ModelDescriptor& descriptor, const TensorSpec& input, std::string& error)
{
    if (input.element_type != TensorElementType::Float32 || input.shape.size() != 4 || (input.shape[0] != 1 && input.shape[0] != -1)
        || input.shape[1] != 3 || input.shape[2] == 0 || input.shape[2] < -1 || input.shape[3] == 0 || input.shape[3] < -1) {
        error = "detector image input must be FP32 NCHW with shape [1, 3, H, W]";
        return false;
    }

    const int64_t model_height = input.shape[2];
    const int64_t model_width = input.shape[3];
    if (descriptor.input.size.width > 0 && descriptor.input.size.height > 0) {
        if ((model_width > 0 && model_width != descriptor.input.size.width)
            || (model_height > 0 && model_height != descriptor.input.size.height)) {
            error = "input_protocol.size does not match the model input shape";
            return false;
        }
    }
    else if (
        model_width > 0 && model_height > 0 && model_width <= std::numeric_limits<int>::max()
        && model_height <= std::numeric_limits<int>::max()) {
        descriptor.input.size = { static_cast<int>(model_width), static_cast<int>(model_height) };
    }
    else {
        error = "dynamic detector input H/W requires input_protocol.size";
        return false;
    }
    return true;
}

bool validate_original_size_input(const TensorSpec& input, std::string& error)
{
    if (input.element_type != TensorElementType::Int64 || input.shape.size() != 2 || (input.shape[0] != 1 && input.shape[0] != -1)
        || input.shape[1] != 2) {
        error = "original_size input must be INT64 with shape [1, 2]";
        return false;
    }
    return true;
}

bool select_outputs(
    const ModelDescriptor& descriptor,
    std::span<const TensorSpec> available,
    std::vector<TensorSpec>& selected,
    std::string& error)
{
    const bool single = descriptor.output.adapter.protocol == OutputProtocol::YoloDense
                        || descriptor.output.adapter.protocol == OutputProtocol::YoloEndToEnd
                        || (descriptor.output.adapter.protocol == OutputProtocol::DetrPacked
                            && descriptor.output.adapter.detr_packed_layout == DetrPackedLayout::Interleaved);
    if (single) {
        if (available.size() != 1) {
            error = "selected output protocol requires exactly one model output";
            return false;
        }
        if (!descriptor.output.output_names.empty() && descriptor.output.output_names.front() != available.front().name) {
            error = "configured output_name does not match the model output";
            return false;
        }
        selected = { available.front() };
        return true;
    }

    if (available.size() != descriptor.output.output_names.size()) {
        error = "model output count does not match output_protocol";
        return false;
    }
    selected.clear();
    selected.reserve(descriptor.output.output_names.size());
    for (const auto& name : descriptor.output.output_names) {
        const auto* tensor = find_tensor(available, name);
        if (!tensor) {
            error = "configured detector output was not found: " + name;
            return false;
        }
        selected.emplace_back(*tensor);
    }
    return true;
}

bool preprocess(const cv::Mat& image, const InputProtocolSpec& spec, PreprocessedImage& output, std::string& error)
{
    if (image.empty() || image.type() != CV_8UC3) {
        error = "detector input image must be non-empty CV_8UC3";
        return false;
    }
    if (spec.size.width <= 0 || spec.size.height <= 0) {
        error = "detector input size must be positive";
        return false;
    }

    const bool letterbox = spec.type == InputProtocol::UltralyticsLetterbox;
    const int interpolation = spec.legacy ? cv::INTER_AREA : cv::INTER_LINEAR;
    cv::Mat resized;
    CoordinateTransform transform {
        .source_size = image.size(),
        .input_size = spec.size,
    };
    if (!letterbox) {
        cv::resize(image, resized, spec.size, 0.0, 0.0, interpolation);
        transform.scale_x = static_cast<float>(spec.size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(spec.size.height) / static_cast<float>(image.rows);
    }
    else {
        const float scale = std::min(
            static_cast<float>(spec.size.width) / static_cast<float>(image.cols),
            static_cast<float>(spec.size.height) / static_cast<float>(image.rows));
        const cv::Size resized_size {
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.cols) * scale))),
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.rows) * scale))),
        };
        cv::Mat scaled;
        cv::resize(image, scaled, resized_size, 0.0, 0.0, interpolation);
        const int left = (spec.size.width - resized_size.width) / 2;
        const int top = (spec.size.height - resized_size.height) / 2;
        const int right = spec.size.width - resized_size.width - left;
        const int bottom = spec.size.height - resized_size.height - top;
        cv::copyMakeBorder(scaled, resized, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
        transform.scale_x = static_cast<float>(resized_size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(resized_size.height) / static_cast<float>(image.rows);
        transform.padding_x = static_cast<float>(left);
        transform.padding_y = static_cast<float>(top);
    }

    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    cv::Mat float_image;
    resized.convertTo(float_image, CV_32FC3, 1.0 / 255.0);
    std::array<cv::Mat, 3> channels;
    cv::split(float_image, channels);

    constexpr std::array<float, 3> kImageNetMean = { 0.485F, 0.456F, 0.406F };
    constexpr std::array<float, 3> kImageNetStd = { 0.229F, 0.224F, 0.225F };
    const bool image_net = spec.type == InputProtocol::DirectResizeImageNet;
    const size_t plane_size = static_cast<size_t>(spec.size.area());
    output.tensor.resize(plane_size * channels.size());
    for (size_t channel = 0; channel < channels.size(); ++channel) {
        if (image_net) {
            channels[channel] = (channels[channel] - kImageNetMean[channel]) / kImageNetStd[channel];
        }
        std::copy_n(channels[channel].ptr<float>(), plane_size, output.tensor.begin() + static_cast<std::ptrdiff_t>(channel * plane_size));
    }
    output.shape = { 1, 3, spec.size.height, spec.size.width };
    output.transform = transform;
    error.clear();
    return true;
}

cv::Rect2f decode_box(const RawDetection& raw, BoxEncoding encoding, const CoordinateTransform& transform)
{
    cv::Rect2f input_box;
    if (encoding == BoxEncoding::InputCxcywh || encoding == BoxEncoding::NormalizedCxcywh) {
        float center_x = raw.box[0];
        float center_y = raw.box[1];
        float width = raw.box[2];
        float height = raw.box[3];
        if (encoding == BoxEncoding::NormalizedCxcywh) {
            center_x *= static_cast<float>(transform.input_size.width);
            center_y *= static_cast<float>(transform.input_size.height);
            width *= static_cast<float>(transform.input_size.width);
            height *= static_cast<float>(transform.input_size.height);
        }
        input_box = { center_x - width / 2.0F, center_y - height / 2.0F, width, height };
    }
    else {
        input_box = { raw.box[0], raw.box[1], raw.box[2] - raw.box[0], raw.box[3] - raw.box[1] };
    }
    return encoding == BoxEncoding::SourceXyxy ? input_box : transform.to_source(input_box);
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
    const auto& selected = !pipeline.empty() ? pipeline : (!descriptor.empty() ? descriptor : metadata);
    if (selected.empty()) {
        output.clear();
        error.clear();
        return true;
    }
    return parse_labels(json::value(selected), output, error);
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
        ModelDescriptor descriptor;
        std::filesystem::path descriptor_path = onnx_path;
        descriptor_path.replace_extension(".json");
        if (std::filesystem::exists(descriptor_path)) {
            const auto descriptor_json = json::open(descriptor_path, true, false);
            if (!descriptor_json) {
                return { .error = "failed to parse model descriptor: " + descriptor_path.string() };
            }
            std::string error;
            if (!parse_descriptor(*descriptor_json, descriptor, error)) {
                return { .error = "invalid model descriptor " + descriptor_path.string() + ": " + error };
            }
        }
        else {
            descriptor = legacy_descriptor();
        }

        auto adapter = NeuralNetworkAdapter::create(descriptor.output.adapter);
        if (!adapter) {
            return { .error = "failed to create detector output adapter" };
        }

        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<TensorSpec> input_specs;
        input_specs.reserve(session->GetInputCount());
        for (size_t index = 0; index < session->GetInputCount(); ++index) {
            const auto info = session->GetInputTypeInfo(index).GetTensorTypeAndShapeInfo();
            const auto element_type = tensor_element_type(info.GetElementType());
            if (!element_type) {
                return { .error = "detector input uses an unsupported tensor element type" };
            }
            input_specs.emplace_back(
                TensorSpec {
                    .name = session->GetInputNameAllocated(index, allocator).get(),
                    .element_type = *element_type,
                    .shape = info.GetShape(),
                });
        }

        const size_t expected_input_count = descriptor.input.original_size ? 2 : 1;
        if (input_specs.size() != expected_input_count) {
            return { .error = "detector input count does not match input_protocol" };
        }
        const TensorSpec* image_input = nullptr;
        if (descriptor.input.name.empty()) {
            image_input = &input_specs.front();
            descriptor.input.name = image_input->name;
        }
        else {
            image_input = find_tensor(input_specs, descriptor.input.name);
        }
        if (!image_input) {
            return { .error = "configured detector image input was not found: " + descriptor.input.name };
        }
        std::string error;
        if (!validate_image_input(descriptor, *image_input, error)) {
            return { .error = "detector input contract mismatch: " + error };
        }

        std::vector<std::string> input_names = { descriptor.input.name };
        if (descriptor.input.original_size) {
            const auto* original_input = find_tensor(input_specs, descriptor.input.original_size->name);
            if (!original_input || !validate_original_size_input(*original_input, error)) {
                return { .error = "detector input contract mismatch: " + (original_input ? error : "original_size input was not found") };
            }
            input_names.emplace_back(descriptor.input.original_size->name);
        }

        std::vector<TensorSpec> available_outputs;
        available_outputs.reserve(session->GetOutputCount());
        for (size_t index = 0; index < session->GetOutputCount(); ++index) {
            const auto info = session->GetOutputTypeInfo(index).GetTensorTypeAndShapeInfo();
            const auto element_type = tensor_element_type(info.GetElementType());
            if (!element_type) {
                return { .error = "detector output uses an unsupported tensor element type" };
            }
            available_outputs.emplace_back(
                TensorSpec {
                    .name = session->GetOutputNameAllocated(index, allocator).get(),
                    .element_type = *element_type,
                    .shape = info.GetShape(),
                });
        }

        std::vector<TensorSpec> outputs;
        if (!select_outputs(descriptor, available_outputs, outputs, error) || !adapter->validate(outputs, error)) {
            return { .error = "detector output contract mismatch: " + error };
        }
        std::vector<std::string> output_names;
        std::vector<TensorElementType> output_types;
        output_names.reserve(outputs.size());
        output_types.reserve(outputs.size());
        for (const auto& output : outputs) {
            output_names.emplace_back(output.name);
            output_types.emplace_back(output.element_type);
        }

        auto model = std::make_shared<Model>();
        model->onnx_path = onnx_path;
        model->session = std::move(session);
        model->descriptor = std::move(descriptor);
        model->adapter = std::shared_ptr<const IAdapter>(std::move(adapter));
        model->metadata_labels = model->descriptor.labels.empty() ? read_metadata_labels(*model->session) : std::vector<std::string> { };
        model->input_names = std::move(input_names);
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
    const double decode_score_floor =
        model_->descriptor.input.legacy ? NeuralNetworkDetectorParam::kDefaultThreshold : threshold_policy.score_floor;

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
        if (!preprocess(image_with_roi(), model_->descriptor.input, preprocessed, error)) {
            return false;
        }

        std::array<int64_t, 2> original_size { };
        std::array<int64_t, 2> original_size_shape = { 1, 2 };
        std::vector<Ort::Value> input_tensors;
        input_tensors.reserve(model_->input_names.size());
        input_tensors.emplace_back(
            Ort::Value::CreateTensor<float>(
                model_->memory_info,
                preprocessed.tensor.data(),
                preprocessed.tensor.size(),
                preprocessed.shape.data(),
                preprocessed.shape.size()));
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
            else {
                view.int64_data = { output_tensors[index].GetTensorData<int64_t>(), info.GetElementCount() };
            }
            tensors.emplace_back(std::move(view));
        }

        DecodedDetections decoded;
        if (!model_->adapter->decode(
                tensors,
                NeuralNetworkAdapter::DecodeOptions { .score_floor = static_cast<float>(score_floor) },
                decoded,
                error)) {
            return false;
        }

        std::vector<DetectionCandidate> candidates;
        candidates.reserve(decoded.candidates.size());
        for (const auto& raw : decoded.candidates) {
            candidates.emplace_back(
                DetectionCandidate {
                    .box = decode_box(raw, model_->descriptor.output.box_encoding, preprocessed.transform),
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

        const auto nms = param_.nms.value_or(model_->descriptor.nms);
        const float nms_threshold = static_cast<float>(param_.nms_threshold.value_or(model_->descriptor.nms_threshold));
        candidates = apply_nms(std::move(candidates), nms, nms_threshold);

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
