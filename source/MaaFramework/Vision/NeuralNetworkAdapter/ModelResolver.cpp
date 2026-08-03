#include "ModelResolver.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <functional>
#include <limits>
#include <ranges>
#include <unordered_set>

#include "MaaUtils/Logger.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

struct InputOverrides
{
    bool present = false;
    bool has_resize = false;
    bool has_color = false;
    bool has_scale = false;
    bool has_layout = false;
    InputPipelineSpec spec;

    bool complete() const { return has_resize && has_color && has_scale && has_layout; }
};

struct OutputOverrides
{
    bool present = false;
    bool has_tensor_map = false;
    bool has_box_decode = false;
    bool has_score_decode = false;
    OutputPipeline spec;
    std::optional<NeuralNetwork::NmsPolicy> preset_nms;

    bool complete() const { return has_tensor_map && has_box_decode && has_score_decode; }
};

struct DescriptorOverrides
{
    InputOverrides input;
    OutputOverrides output;
    std::vector<std::string> labels;
    std::optional<NeuralNetwork::NmsPolicy> nms;
    std::optional<float> nms_threshold;
};

enum class ProfileState
{
    Unrecognized,
    Valid,
    Incompatible,
};

struct MetadataProfile
{
    ProfileState state = ProfileState::Unrecognized;
    std::optional<InputPipelineSpec> input;
    std::optional<OutputPipeline> output;
    std::optional<cv::Size> input_size;
    std::optional<NeuralNetwork::NmsPolicy> nms;
    std::optional<float> nms_threshold;
    std::string error;
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

    if (!validate_labels(parsed, error)) {
        return false;
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

const std::string* metadata_value(const ModelFacts& facts, std::string_view key)
{
    const auto iterator = facts.metadata.find(std::string(key));
    return iterator == facts.metadata.end() ? nullptr : &iterator->second;
}

std::vector<std::string> metadata_labels(const ModelFacts& facts)
{
    constexpr std::array<std::string_view, 4> kKeys = { "names", "name", "labels", "class_names" };
    for (const auto key : kKeys) {
        const auto* raw = metadata_value(facts, key);
        if (!raw) {
            continue;
        }
        std::vector<std::string> labels;
        std::string error;
        if (parse_metadata_labels(*raw, labels, error)) {
            return labels;
        }
        LogWarn << "Failed to parse ONNX label metadata" << VAR(key) << VAR(error);
    }
    return { };
}

bool parse_metadata_size(std::string_view text, cv::Size& output)
{
    std::vector<int> values;
    for (size_t position = 0; position < text.size();) {
        if (!std::isdigit(static_cast<unsigned char>(text[position]))) {
            ++position;
            continue;
        }
        int value = 0;
        const auto* begin = text.data() + position;
        const auto* end = text.data() + text.size();
        const auto [parsed_end, conversion_error] = std::from_chars(begin, end, value);
        if (conversion_error != std::errc() || value <= 0) {
            return false;
        }
        values.emplace_back(value);
        position = static_cast<size_t>(parsed_end - text.data());
    }
    if (values.size() == 1) {
        output = { values.front(), values.front() };
        return true;
    }
    if (values.size() == 2) {
        output = { values[1], values[0] };
        return true;
    }
    return false;
}

bool parse_boolean(std::string_view text, bool& output)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    std::string normalized(text);
    std::ranges::transform(normalized, normalized.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (normalized == "true" || normalized == "1") {
        output = true;
        return true;
    }
    if (normalized == "false" || normalized == "0") {
        output = false;
        return true;
    }
    return false;
}

bool parse_args_nms(std::string_view text, bool& output)
{
    if (auto value = json::parse(text); value && value->is_object()) {
        if (auto nms = value->find<bool>("nms")) {
            output = *nms;
            return true;
        }
        return false;
    }

    for (size_t position = 0; position + 5 <= text.size(); ++position) {
        const char quote = text[position];
        if ((quote != '\'' && quote != '"') || text.substr(position + 1, 3) != "nms" || text[position + 4] != quote) {
            continue;
        }
        position += 5;
        skip_spaces(text, position);
        if (position == text.size() || text[position++] != ':') {
            return false;
        }
        skip_spaces(text, position);
        const size_t begin = position;
        while (position < text.size() && text[position] != ',' && text[position] != '}') {
            ++position;
        }
        return parse_boolean(text.substr(begin, position - begin), output);
    }
    return false;
}

InputPipelineSpec resize_preset(bool image_net)
{
    InputPipelineSpec spec;
    spec.resize_mode = ResizeMode::Stretch;
    spec.interpolation = ResizeInterpolation::Linear;
    spec.color = ColorOrder::RGB;
    spec.scale = 1.0F / 255.0F;
    spec.layout = ImageLayout::NCHW;
    if (image_net) {
        spec.mean = { 0.485F, 0.456F, 0.406F };
        spec.std = { 0.229F, 0.224F, 0.225F };
    }
    return spec;
}

InputPipelineSpec ultralytics_input_preset()
{
    InputPipelineSpec spec = resize_preset(false);
    spec.resize_mode = ResizeMode::Letterbox;
    spec.fill = { 114.0F, 114.0F, 114.0F };
    spec.padding_position = PaddingPosition::Center;
    return spec;
}

OutputPipeline yolo_dense_preset()
{
    return OutputPipeline {
        .adapter =
            OutputPipelineSpec {
                .tensor_map =
                    TensorMapSpec {
                        .layout = TensorLayout::ChannelsFirst,
                        .fields = { TensorField::Boxes, TensorField::ClassScores },
                    },
                .score_decode = ScoreDecodeSpec { },
            },
        .box_decode = BoxDecodeSpec { .format = BoxFormat::CxCyWh, .coordinates = CoordinateSpace::Input },
    };
}

OutputPipeline yolo_end_to_end_preset()
{
    return OutputPipeline {
        .adapter =
            OutputPipelineSpec {
                .tensor_map =
                    TensorMapSpec {
                        .layout = TensorLayout::ChannelsLast,
                        .fields = { TensorField::Boxes, TensorField::Score, TensorField::ClassId },
                    },
                .score_decode = ScoreDecodeSpec { },
            },
        .box_decode = BoxDecodeSpec { .format = BoxFormat::XyXy, .coordinates = CoordinateSpace::Input },
    };
}

OutputPipeline detr_queries_preset()
{
    return OutputPipeline {
        .adapter =
            OutputPipelineSpec {
                .tensor_map =
                    TensorMapSpec {
                        .layout = TensorLayout::Split,
                        .fields = { TensorField::Boxes, TensorField::ClassScores },
                    },
                .score_decode = ScoreDecodeSpec { .activation = ScoreActivation::Sigmoid },
                .top_k = 300,
            },
        .box_decode = BoxDecodeSpec { .format = BoxFormat::CxCyWh, .coordinates = CoordinateSpace::Normalized },
    };
}

OutputPipeline detr_packed_preset()
{
    return OutputPipeline {
        .adapter =
            OutputPipelineSpec {
                .tensor_map =
                    TensorMapSpec {
                        .layout = TensorLayout::ChannelsLast,
                        .fields = { TensorField::Boxes, TensorField::Score, TensorField::ClassId },
                    },
                .score_decode = ScoreDecodeSpec { },
            },
        .box_decode = BoxDecodeSpec { .format = BoxFormat::CxCyWh, .coordinates = CoordinateSpace::Normalized },
    };
}

bool parse_size(const json::value& step, std::string_view key, std::optional<cv::Size>& output, std::string& error)
{
    const std::string key_text(key);
    if (!step.exists(key_text)) {
        return true;
    }
    const auto values = step.find<std::vector<int>>(key_text);
    if (!values || values->size() != 2 || (*values)[0] <= 0 || (*values)[1] <= 0) {
        error = std::string(key) + " must be [width, height] with positive integers";
        return false;
    }
    output = cv::Size { (*values)[0], (*values)[1] };
    return true;
}

bool parse_triplet(const json::value& step, std::string_view key, std::array<float, 3>& output, std::string& error)
{
    const auto values = step.find<std::vector<double>>(std::string(key));
    if (!values || values->size() != 3 || std::ranges::any_of(*values, [](double value) { return !std::isfinite(value); })) {
        error = std::string(key) + " must contain three finite numbers";
        return false;
    }
    std::ranges::transform(*values, output.begin(), [](double value) { return static_cast<float>(value); });
    return true;
}

bool apply_input_preset(std::string_view preset, InputOverrides& output, std::string& error)
{
    if (preset == "UltralyticsLetterbox") {
        output.spec = ultralytics_input_preset();
    }
    else if (preset == "Resize") {
        output.spec = resize_preset(false);
    }
    else if (preset == "ResizeIN") {
        output.spec = resize_preset(true);
    }
    else {
        error = "input preset is not supported: " + std::string(preset);
        return false;
    }
    output.has_resize = true;
    output.has_color = true;
    output.has_scale = true;
    output.has_layout = true;
    return true;
}

bool parse_input_step(const json::value& step, InputOverrides& output, std::string& error)
{
    const auto type = step.find<std::string>("type");
    if (!type) {
        error = step.exists("type") ? "input step type must be a string" : "input step type is required";
        return false;
    }
    if (*type == "Resize") {
        static const std::unordered_set<std::string_view> kAllowed = {
            "type", "mode", "size", "interpolation", "fill", "padding_position"
        };
        if (!check_allowed_fields(step.as_object(), kAllowed, "Resize", error)) {
            return false;
        }
        InputPipelineSpec defaults;
        output.spec.resize_mode = ResizeMode::Stretch;
        output.spec.resize_size.reset();
        output.spec.interpolation = ResizeInterpolation::Linear;
        output.spec.fill = defaults.fill;
        output.spec.padding_position = PaddingPosition::Center;
        if (auto mode = step.find<std::string>("mode")) {
            if (*mode == "Stretch") {
                output.spec.resize_mode = ResizeMode::Stretch;
            }
            else if (*mode == "Letterbox") {
                output.spec.resize_mode = ResizeMode::Letterbox;
            }
            else {
                error = "Resize mode must be Stretch or Letterbox";
                return false;
            }
        }
        else if (step.exists("mode")) {
            error = "Resize mode must be a string";
            return false;
        }
        if (!parse_size(step, "size", output.spec.resize_size, error)) {
            return false;
        }
        if (auto interpolation = step.find<std::string>("interpolation")) {
            if (*interpolation == "Nearest") {
                output.spec.interpolation = ResizeInterpolation::Nearest;
            }
            else if (*interpolation == "Linear") {
                output.spec.interpolation = ResizeInterpolation::Linear;
            }
            else if (*interpolation == "Cubic") {
                output.spec.interpolation = ResizeInterpolation::Cubic;
            }
            else if (*interpolation == "Area") {
                output.spec.interpolation = ResizeInterpolation::Area;
            }
            else {
                error = "Resize interpolation is not supported";
                return false;
            }
        }
        else if (step.exists("interpolation")) {
            error = "Resize interpolation must be a string";
            return false;
        }
        if (step.exists("fill")) {
            if (auto scalar = step.find<double>("fill")) {
                if (!std::isfinite(*scalar) || *scalar < 0.0 || *scalar > 255.0) {
                    error = "Resize fill must be in [0, 255]";
                    return false;
                }
                output.spec.fill.fill(static_cast<float>(*scalar));
            }
            else if (!parse_triplet(step, "fill", output.spec.fill, error) || std::ranges::any_of(output.spec.fill, [](float value) {
                         return value < 0.0F || value > 255.0F;
                     })) {
                error = "Resize fill must be a number or three numbers in [0, 255]";
                return false;
            }
        }
        if (auto position = step.find<std::string>("padding_position")) {
            if (*position == "Center") {
                output.spec.padding_position = PaddingPosition::Center;
            }
            else if (*position == "TopLeft") {
                output.spec.padding_position = PaddingPosition::TopLeft;
            }
            else {
                error = "Resize padding_position must be Center or TopLeft";
                return false;
            }
        }
        else if (step.exists("padding_position")) {
            error = "Resize padding_position must be a string";
            return false;
        }
        output.has_resize = true;
        return true;
    }
    if (*type == "Color") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "order" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "Color", error)) {
            return false;
        }
        const auto order_value = step.find<std::string>("order");
        if (!order_value && step.exists("order")) {
            error = "Color order must be a string";
            return false;
        }
        const auto order = order_value.value_or("RGB");
        if (order == "RGB") {
            output.spec.color = ColorOrder::RGB;
        }
        else if (order == "BGR") {
            output.spec.color = ColorOrder::BGR;
        }
        else {
            error = "Color order must be RGB or BGR";
            return false;
        }
        output.has_color = true;
        return true;
    }
    if (*type == "Scale") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "factor" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "Scale", error)) {
            return false;
        }
        const auto factor_value = step.find<double>("factor");
        if (!factor_value && step.exists("factor")) {
            error = "Scale factor must be a number";
            return false;
        }
        const auto factor = factor_value.value_or(1.0 / 255.0);
        if (!std::isfinite(factor)) {
            error = "Scale factor must be finite";
            return false;
        }
        output.spec.scale = static_cast<float>(factor);
        output.has_scale = true;
        return true;
    }
    if (*type == "Normalize") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "mean", "std" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "Normalize", error) || !step.exists("mean") || !step.exists("std")
            || !parse_triplet(step, "mean", output.spec.mean, error) || !parse_triplet(step, "std", output.spec.std, error)) {
            if (error.empty()) {
                error = "Normalize mean and std are required";
            }
            return false;
        }
        if (std::ranges::any_of(output.spec.std, [](float value) { return value == 0.0F; })) {
            error = "Normalize std must not contain zero";
            return false;
        }
        return true;
    }
    if (*type == "Layout") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "order" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "Layout", error)) {
            return false;
        }
        const auto order_value = step.find<std::string>("order");
        if (!order_value && step.exists("order")) {
            error = "Layout order must be a string";
            return false;
        }
        const auto order = order_value.value_or("NCHW");
        if (order == "NCHW") {
            output.spec.layout = ImageLayout::NCHW;
        }
        else if (order == "NHWC") {
            output.spec.layout = ImageLayout::NHWC;
        }
        else {
            error = "Layout order must be NCHW or NHWC";
            return false;
        }
        output.has_layout = true;
        return true;
    }
    if (*type == "AuxInput") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "kind", "order" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "AuxInput", error)) {
            return false;
        }
        const auto kind_value = step.find<std::string>("kind");
        if (!kind_value && step.exists("kind")) {
            error = "AuxInput kind must be a string";
            return false;
        }
        const auto kind = kind_value.value_or("OriginalSize");
        if (kind != "OriginalSize") {
            error = "AuxInput kind must be OriginalSize";
            return false;
        }
        const auto order_value = step.find<std::string>("order");
        if (!order_value && step.exists("order")) {
            error = "AuxInput order must be a string";
            return false;
        }
        const auto order = order_value.value_or("HW");
        if (order != "HW" && order != "WH") {
            error = "AuxInput order must be HW or WH";
            return false;
        }
        output.spec.original_size = OriginalSizeInput {
            .order = order == "HW" ? OriginalSizeOrder::HW : OriginalSizeOrder::WH,
        };
        return true;
    }
    error = "input step type is not supported: " + *type;
    return false;
}

bool parse_input(const json::value& value, InputOverrides& output, std::string& error)
{
    if (!value.is_array()) {
        error = "input must be an array";
        return false;
    }
    if (value.as_array().empty()) {
        error = "input must contain at least one step";
        return false;
    }
    InputOverrides parsed;
    parsed.present = true;
    std::unordered_set<std::string> stages;
    bool has_preset = false;
    for (const auto& step : value.as_array()) {
        if (!step.is_object()) {
            error = "input steps must be objects";
            return false;
        }
        if (!step.exists("preset")) {
            continue;
        }
        static const std::unordered_set<std::string_view> kAllowed = { "preset" };
        if (has_preset || !check_allowed_fields(step.as_object(), kAllowed, "input preset", error)) {
            error = has_preset ? "input may contain at most one preset" : error;
            return false;
        }
        const auto preset = step.find<std::string>("preset");
        if (!preset || !apply_input_preset(*preset, parsed, error)) {
            if (!preset) {
                error = "input preset must be a string";
            }
            return false;
        }
        has_preset = true;
    }
    for (const auto& step : value.as_array()) {
        if (step.exists("preset")) {
            continue;
        }
        const auto type = step.find<std::string>("type");
        if (!type) {
            error = step.exists("type") ? "input step type must be a string" : "input step requires type or preset";
            return false;
        }
        if (!stages.emplace(*type).second) {
            error = "duplicate input stage: " + *type;
            return false;
        }
        if (!parse_input_step(step, parsed, error)) {
            return false;
        }
    }
    output = std::move(parsed);
    error.clear();
    return true;
}

bool apply_output_preset(std::string_view preset, OutputOverrides& output, std::string& error)
{
    if (preset == "YoloDense") {
        output.spec = yolo_dense_preset();
        output.preset_nms = NeuralNetwork::NmsPolicy::ClassAwareIoU;
    }
    else if (preset == "YoloEndToEnd") {
        output.spec = yolo_end_to_end_preset();
        output.preset_nms = NeuralNetwork::NmsPolicy::None;
    }
    else if (preset == "DetrQueries") {
        output.spec = detr_queries_preset();
        output.preset_nms = NeuralNetwork::NmsPolicy::None;
    }
    else if (preset == "DetrPacked") {
        output.spec = detr_packed_preset();
        output.preset_nms = NeuralNetwork::NmsPolicy::None;
    }
    else {
        error = "output preset is not supported: " + std::string(preset);
        return false;
    }
    output.has_tensor_map = true;
    output.has_box_decode = true;
    output.has_score_decode = true;
    return true;
}

std::optional<TensorField> parse_tensor_field(std::string_view value)
{
    if (value == "Boxes") {
        return TensorField::Boxes;
    }
    if (value == "ClassScores") {
        return TensorField::ClassScores;
    }
    if (value == "Objectness") {
        return TensorField::Objectness;
    }
    if (value == "Score") {
        return TensorField::Score;
    }
    if (value == "ClassId") {
        return TensorField::ClassId;
    }
    return std::nullopt;
}

std::optional<ScoreActivation> parse_activation(std::string_view value)
{
    if (value == "None") {
        return ScoreActivation::None;
    }
    if (value == "Sigmoid") {
        return ScoreActivation::Sigmoid;
    }
    if (value == "Softmax") {
        return ScoreActivation::Softmax;
    }
    return std::nullopt;
}

bool parse_output_step(const json::value& step, OutputOverrides& output, std::string& error)
{
    const auto type = step.find<std::string>("type");
    if (!type) {
        error = step.exists("type") ? "output step type must be a string" : "output step type is required";
        return false;
    }
    if (*type == "TensorMap") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "layout", "fields" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "TensorMap", error)) {
            return false;
        }
        const auto layout = step.find<std::string>("layout");
        const auto fields = step.find<std::vector<std::string>>("fields");
        if (!layout || !fields || fields->empty()) {
            error = "TensorMap layout and non-empty fields are required";
            return false;
        }
        if (*layout == "ChannelsFirst") {
            output.spec.adapter.tensor_map.layout = TensorLayout::ChannelsFirst;
        }
        else if (*layout == "ChannelsLast") {
            output.spec.adapter.tensor_map.layout = TensorLayout::ChannelsLast;
        }
        else if (*layout == "Split") {
            output.spec.adapter.tensor_map.layout = TensorLayout::Split;
        }
        else {
            error = "TensorMap layout must be ChannelsFirst, ChannelsLast, or Split";
            return false;
        }
        output.spec.adapter.tensor_map.fields.clear();
        for (const auto& field : *fields) {
            const auto parsed = parse_tensor_field(field);
            if (!parsed) {
                error = "TensorMap field is not supported: " + field;
                return false;
            }
            output.spec.adapter.tensor_map.fields.emplace_back(*parsed);
        }
        output.has_tensor_map = true;
        return true;
    }
    if (*type == "BoxDecode") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "format", "coordinates" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "BoxDecode", error)) {
            return false;
        }
        const auto format = step.find<std::string>("format");
        const auto coordinates = step.find<std::string>("coordinates");
        if (!format || !coordinates) {
            error = "BoxDecode format and coordinates are required";
            return false;
        }
        if (*format == "CxCyWh") {
            output.spec.box_decode.format = BoxFormat::CxCyWh;
        }
        else if (*format == "XyXy") {
            output.spec.box_decode.format = BoxFormat::XyXy;
        }
        else {
            error = "BoxDecode format must be CxCyWh or XyXy";
            return false;
        }
        if (*coordinates == "Input") {
            output.spec.box_decode.coordinates = CoordinateSpace::Input;
        }
        else if (*coordinates == "Normalized") {
            output.spec.box_decode.coordinates = CoordinateSpace::Normalized;
        }
        else if (*coordinates == "Source") {
            output.spec.box_decode.coordinates = CoordinateSpace::Source;
        }
        else {
            error = "BoxDecode coordinates must be Input, Normalized, or Source";
            return false;
        }
        output.has_box_decode = true;
        return true;
    }
    if (*type == "ScoreDecode") {
        static const std::unordered_set<std::string_view> kAllowed = { "type",
                                                                       "activation",
                                                                       "objectness_activation",
                                                                       "combine",
                                                                       "no_object_index" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "ScoreDecode", error)) {
            return false;
        }
        ScoreDecodeSpec score;
        if (auto activation = step.find<std::string>("activation")) {
            const auto parsed = parse_activation(*activation);
            if (!parsed) {
                error = "ScoreDecode activation must be None, Sigmoid, or Softmax";
                return false;
            }
            score.activation = *parsed;
        }
        else if (step.exists("activation")) {
            error = "ScoreDecode activation must be a string";
            return false;
        }
        if (auto activation = step.find<std::string>("objectness_activation")) {
            const auto parsed = parse_activation(*activation);
            if (!parsed) {
                error = "ScoreDecode objectness_activation must be None, Sigmoid, or Softmax";
                return false;
            }
            score.objectness_activation = *parsed;
        }
        else if (step.exists("objectness_activation")) {
            error = "ScoreDecode objectness_activation must be a string";
            return false;
        }
        if (auto combine = step.find<std::string>("combine")) {
            if (*combine == "None") {
                score.combination = ScoreCombination::None;
            }
            else if (*combine == "Multiply") {
                score.combination = ScoreCombination::Multiply;
            }
            else {
                error = "ScoreDecode combine must be None or Multiply";
                return false;
            }
        }
        else if (step.exists("combine")) {
            error = "ScoreDecode combine must be a string";
            return false;
        }
        if (step.exists("no_object_index")) {
            if (auto index = step.find<int64_t>("no_object_index")) {
                if (*index < 0) {
                    error = "ScoreDecode no_object_index integer must be non-negative";
                    return false;
                }
                score.no_object_index = *index;
            }
            else if (auto named_index = step.find<std::string>("no_object_index")) {
                if (*named_index == "First") {
                    score.no_object_index = 0;
                }
                else if (*named_index == "Last") {
                    score.no_object_index = -1;
                }
                else {
                    error = "ScoreDecode no_object_index must be First, Last, or a non-negative integer";
                    return false;
                }
            }
            else {
                error = "ScoreDecode no_object_index must be First, Last, or a non-negative integer";
                return false;
            }
        }
        output.spec.adapter.score_decode = score;
        output.has_score_decode = true;
        return true;
    }
    if (*type == "CandidateSelect") {
        static const std::unordered_set<std::string_view> kAllowed = { "type", "top_k" };
        if (!check_allowed_fields(step.as_object(), kAllowed, "CandidateSelect", error)) {
            return false;
        }
        output.spec.adapter.top_k.reset();
        if (step.exists("top_k")) {
            const auto top_k = step.find<int64_t>("top_k");
            if (!top_k || *top_k <= 0 || static_cast<uint64_t>(*top_k) > std::numeric_limits<size_t>::max()) {
                error = "CandidateSelect top_k must be a positive integer";
                return false;
            }
            output.spec.adapter.top_k = static_cast<size_t>(*top_k);
        }
        return true;
    }
    error = "output step type is not supported: " + *type;
    return false;
}

bool parse_output(const json::value& value, OutputOverrides& output, std::string& error)
{
    if (!value.is_array()) {
        error = "output must be an array";
        return false;
    }
    if (value.as_array().empty()) {
        error = "output must contain at least one step";
        return false;
    }
    OutputOverrides parsed;
    parsed.present = true;
    std::unordered_set<std::string> stages;
    bool has_preset = false;
    for (const auto& step : value.as_array()) {
        if (!step.is_object()) {
            error = "output steps must be objects";
            return false;
        }
        if (!step.exists("preset")) {
            continue;
        }
        static const std::unordered_set<std::string_view> kAllowed = { "preset" };
        if (has_preset || !check_allowed_fields(step.as_object(), kAllowed, "output preset", error)) {
            error = has_preset ? "output may contain at most one preset" : error;
            return false;
        }
        const auto preset = step.find<std::string>("preset");
        if (!preset || !apply_output_preset(*preset, parsed, error)) {
            if (!preset) {
                error = "output preset must be a string";
            }
            return false;
        }
        has_preset = true;
    }
    for (const auto& step : value.as_array()) {
        if (step.exists("preset")) {
            continue;
        }
        const auto type = step.find<std::string>("type");
        if (!type) {
            error = step.exists("type") ? "output step type must be a string" : "output step requires type or preset";
            return false;
        }
        if (!stages.emplace(*type).second) {
            error = "duplicate output stage: " + *type;
            return false;
        }
        if (!parse_output_step(step, parsed, error)) {
            return false;
        }
    }
    output = std::move(parsed);
    error.clear();
    return true;
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

bool parse_descriptor(const json::value& value, DescriptorOverrides& output, std::string& error)
{
    if (!value.is_object()) {
        error = "model descriptor must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowed = { "$schema", "input", "output", "labels", "nms" };
    if (!check_allowed_fields(value.as_object(), kAllowed, "model descriptor", error)) {
        return false;
    }
    if (value.exists("$schema") && !value.find<std::string>("$schema")) {
        error = "$schema must be a string";
        return false;
    }

    DescriptorOverrides parsed;
    if (auto input = value.find("input"); input && !parse_input(*input, parsed.input, error)) {
        return false;
    }
    if (auto output_steps = value.find("output"); output_steps && !parse_output(*output_steps, parsed.output, error)) {
        return false;
    }
    if (auto labels = value.find("labels"); labels && !parse_labels(*labels, parsed.labels, error)) {
        return false;
    }
    if (auto nms = value.find("nms")) {
        if (!nms->is_object()) {
            error = "nms must be an object";
            return false;
        }
        static const std::unordered_set<std::string_view> kNmsAllowed = { "type", "threshold" };
        if (!check_allowed_fields(nms->as_object(), kNmsAllowed, "nms", error)) {
            return false;
        }
        const auto type = nms->find<std::string>("type");
        NeuralNetwork::NmsPolicy policy;
        if (!type || !parse_nms(*type, policy)) {
            error = "nms.type is required and must be None, ClassAwareIoU, or CandidateCoverage";
            return false;
        }
        parsed.nms = policy;
        if (auto threshold = nms->find<double>("threshold")) {
            if (!std::isfinite(*threshold) || *threshold < 0.0 || *threshold > 1.0) {
                error = "nms.threshold must be in [0, 1]";
                return false;
            }
            if (policy == NeuralNetwork::NmsPolicy::None) {
                LogWarn << "Ignoring nms.threshold because nms.type is None" << VAR(*threshold);
            }
            else {
                parsed.nms_threshold = static_cast<float>(*threshold);
            }
        }
        else if (nms->exists("threshold")) {
            error = "nms.threshold must be a number";
            return false;
        }
    }
    output = std::move(parsed);
    error.clear();
    return true;
}

bool any_tensor_matches(const OutputPipeline& output, const ModelFacts& facts)
{
    if (output.adapter.tensor_map.layout == TensorLayout::Split) {
        return false;
    }
    auto adapter = create(output.adapter);
    for (const auto& tensor : facts.outputs) {
        std::array<TensorSpec, 1> candidate = { tensor };
        std::string error;
        if (adapter->validate(candidate, error)) {
            return true;
        }
    }
    return false;
}

MetadataProfile read_metadata_profile(const ModelFacts& facts)
{
    const auto* author = metadata_value(facts, "author");
    if (!author || *author != "Ultralytics") {
        return { };
    }

    MetadataProfile profile;
    profile.state = ProfileState::Valid;
    const auto* task = metadata_value(facts, "task");
    if (!task) {
        LogWarn << "Ignoring incomplete Ultralytics metadata: task is missing";
        return profile;
    }
    if (*task != "detect") {
        profile.state = ProfileState::Incompatible;
        profile.error = "Ultralytics task metadata is not supported: " + *task;
        return profile;
    }

    if (const auto* imgsz = metadata_value(facts, "imgsz")) {
        cv::Size parsed;
        if (parse_metadata_size(*imgsz, parsed)) {
            profile.input_size = parsed;
        }
        else {
            LogWarn << "Ignoring invalid Ultralytics imgsz metadata" << VAR(*imgsz);
        }
    }

    const auto* head = metadata_value(facts, "head");
    if (head && *head == "RTDETRDecoder") {
        profile.input = resize_preset(false);
        profile.output = detr_packed_preset();
        profile.nms = NeuralNetwork::NmsPolicy::None;
        return profile;
    }
    if (!head || *head != "Detect") {
        if (head) {
            LogWarn << "Ultralytics head does not have an automatic detector profile; model JSON is required" << VAR(*head);
        }
        else {
            LogWarn << "Ultralytics head metadata is missing; model JSON is required";
        }
        return profile;
    }

    profile.input = ultralytics_input_preset();
    std::optional<bool> end2end;
    if (const auto* value = metadata_value(facts, "end2end")) {
        bool parsed = false;
        if (parse_boolean(*value, parsed)) {
            end2end = parsed;
        }
        else {
            LogWarn << "Ignoring invalid Ultralytics end2end metadata" << VAR(*value);
        }
    }
    std::optional<bool> graph_nms;
    if (const auto* value = metadata_value(facts, "args")) {
        bool parsed = false;
        if (parse_args_nms(*value, parsed)) {
            graph_nms = parsed;
        }
        else {
            LogWarn << "Ignoring invalid or missing Ultralytics args.nms metadata";
        }
    }

    if (end2end == true || graph_nms == true) {
        profile.output = yolo_end_to_end_preset();
    }
    else {
        const bool dense_match = any_tensor_matches(yolo_dense_preset(), facts);
        const bool end_to_end_match = any_tensor_matches(yolo_end_to_end_preset(), facts);
        if (dense_match != end_to_end_match) {
            profile.output = dense_match ? yolo_dense_preset() : yolo_end_to_end_preset();
        }
        else if (end2end == false) {
            profile.output = yolo_dense_preset();
        }
    }
    if (profile.output) {
        const bool dense = profile.output->adapter.tensor_map.fields == yolo_dense_preset().adapter.tensor_map.fields;
        profile.nms = dense ? NeuralNetwork::NmsPolicy::ClassAwareIoU : NeuralNetwork::NmsPolicy::None;
        profile.nms_threshold = 0.7F;
    }
    return profile;
}

bool same_input_semantics(const InputPipelineSpec& lhs, const InputPipelineSpec& rhs)
{
    return lhs.resize_mode == rhs.resize_mode && lhs.interpolation == rhs.interpolation && lhs.fill == rhs.fill
           && lhs.padding_position == rhs.padding_position && lhs.color == rhs.color && lhs.scale == rhs.scale && lhs.mean == rhs.mean
           && lhs.std == rhs.std && lhs.layout == rhs.layout && lhs.original_size.has_value() == rhs.original_size.has_value()
           && (!lhs.original_size || lhs.original_size->order == rhs.original_size->order);
}

bool same_output_semantics(const OutputPipeline& lhs, const OutputPipeline& rhs)
{
    return lhs.adapter.tensor_map.layout == rhs.adapter.tensor_map.layout && lhs.adapter.tensor_map.fields == rhs.adapter.tensor_map.fields
           && lhs.adapter.score_decode.activation == rhs.adapter.score_decode.activation
           && lhs.adapter.score_decode.objectness_activation == rhs.adapter.score_decode.objectness_activation
           && lhs.adapter.score_decode.combination == rhs.adapter.score_decode.combination
           && lhs.adapter.score_decode.no_object_index == rhs.adapter.score_decode.no_object_index && lhs.adapter.top_k == rhs.adapter.top_k
           && lhs.box_decode.format == rhs.box_decode.format && lhs.box_decode.coordinates == rhs.box_decode.coordinates;
}

bool resolve_descriptor(const DescriptorOverrides& overrides, const MetadataProfile& profile, ModelDescriptor& output, std::string& error)
{
    if (profile.state == ProfileState::Incompatible) {
        error = profile.error;
        return false;
    }

    if (!profile.input && !overrides.input.complete()) {
        error = "model protocol is incomplete: input processing cannot be determined from metadata or JSON";
        return false;
    }
    if (!profile.output && !overrides.output.complete()) {
        error = "model protocol is incomplete: output decoding cannot be determined from metadata or JSON";
        return false;
    }

    ModelDescriptor descriptor;
    if (profile.input) {
        descriptor.input = *profile.input;
        if (overrides.input.present && overrides.input.complete() && !same_input_semantics(*profile.input, overrides.input.spec)) {
            LogWarn << "Ignoring conflicting model JSON input steps because Ultralytics metadata has priority";
        }
        if (profile.input_size) {
            descriptor.input.resize_size = profile.input_size;
            if (overrides.input.spec.resize_size && *overrides.input.spec.resize_size != *profile.input_size) {
                LogWarn << "Ignoring conflicting model JSON Resize.size because Ultralytics metadata has priority";
            }
        }
        else if (overrides.input.spec.resize_size) {
            descriptor.input.resize_size = overrides.input.spec.resize_size;
        }
    }
    else {
        descriptor.input = overrides.input.spec;
    }

    if (profile.output) {
        descriptor.output = *profile.output;
        if (overrides.output.present && overrides.output.complete() && !same_output_semantics(*profile.output, overrides.output.spec)) {
            LogWarn << "Ignoring conflicting model JSON output steps because Ultralytics metadata has priority";
        }
    }
    else {
        descriptor.output = overrides.output.spec;
    }
    descriptor.labels = overrides.labels;

    if (profile.output && profile.nms) {
        descriptor.nms = *profile.nms;
        if (overrides.nms && *overrides.nms != *profile.nms) {
            LogWarn << "Ignoring conflicting model JSON NMS because Ultralytics metadata has priority";
        }
    }
    else {
        descriptor.nms = overrides.nms.value_or(overrides.output.preset_nms.value_or(NeuralNetwork::NmsPolicy::None));
    }
    if (profile.output && profile.nms_threshold) {
        descriptor.nms_threshold = *profile.nms_threshold;
        if (overrides.nms_threshold && *overrides.nms_threshold != *profile.nms_threshold) {
            LogWarn << "Ignoring conflicting model JSON NMS threshold because Ultralytics metadata has priority";
        }
    }
    else {
        descriptor.nms_threshold = overrides.nms_threshold.value_or(0.7F);
    }
    output = std::move(descriptor);
    error.clear();
    return true;
}

bool valid_spatial_dimension(int64_t value)
{
    return value == -1 || value > 0;
}

bool is_image_input(const TensorSpec& input, ImageLayout layout)
{
    if (!is_float_tensor(input.element_type) || input.shape.size() != 4 || !has_batch_one(input.shape)) {
        return false;
    }
    const size_t channel_axis = layout == ImageLayout::NCHW ? 1 : 3;
    const size_t height_axis = layout == ImageLayout::NCHW ? 2 : 1;
    const size_t width_axis = layout == ImageLayout::NCHW ? 3 : 2;
    return input.shape[channel_axis] == 3 && valid_spatial_dimension(input.shape[height_axis])
           && valid_spatial_dimension(input.shape[width_axis]);
}

bool validate_image_input(ModelDescriptor& descriptor, const TensorSpec& input, std::string& error)
{
    if (!is_image_input(input, descriptor.input.layout)) {
        error = "detector image input must be FP32 or FP16, RGB three-channel, batch-one, and match the configured layout";
        return false;
    }
    const size_t height_axis = descriptor.input.layout == ImageLayout::NCHW ? 2 : 1;
    const size_t width_axis = descriptor.input.layout == ImageLayout::NCHW ? 3 : 2;
    const int64_t height = input.shape[height_axis];
    const int64_t width = input.shape[width_axis];
    if ((height > std::numeric_limits<int>::max()) || (width > std::numeric_limits<int>::max())) {
        error = "detector input spatial size exceeds the supported range";
        return false;
    }
    if (descriptor.input.resize_size
        && ((width > 0 && width != descriptor.input.resize_size->width)
            || (height > 0 && height != descriptor.input.resize_size->height))) {
        LogWarn << "Static ModelProto input size overrides model metadata or JSON Resize.size" << VAR(input.shape);
    }
    descriptor.input.name = input.name;
    descriptor.input.element_type = input.element_type;
    descriptor.input.graph_shape = input.shape;
    error.clear();
    return true;
}

bool is_original_size_input(const TensorSpec& input)
{
    return input.element_type == TensorElementType::Int64 && input.shape.size() == 2 && has_batch_one(input.shape) && input.shape[1] == 2;
}

bool select_outputs(
    const ModelDescriptor& descriptor,
    const IAdapter& adapter,
    std::span<const TensorSpec> available,
    std::vector<TensorSpec>& selected,
    std::string& error)
{
    const size_t expected_count =
        descriptor.output.adapter.tensor_map.layout == TensorLayout::Split ? descriptor.output.adapter.tensor_map.fields.size() : 1;
    if (available.size() < expected_count) {
        error = "model does not expose enough outputs for TensorMap";
        return false;
    }

    size_t match_count = 0;
    std::vector<TensorSpec> candidate;
    candidate.reserve(expected_count);
    std::vector<bool> used(available.size());
    std::function<void()> search = [&]() {
        if (match_count > 1) {
            return;
        }
        if (candidate.size() == expected_count) {
            std::string validation_error;
            if (adapter.validate(candidate, validation_error)) {
                ++match_count;
                selected = candidate;
            }
            return;
        }
        for (size_t index = 0; index < available.size(); ++index) {
            if (used[index]) {
                continue;
            }
            used[index] = true;
            candidate.emplace_back(available[index]);
            search();
            candidate.pop_back();
            used[index] = false;
        }
    };
    search();

    if (match_count == 0) {
        error = "no output tensor set matches TensorMap";
        return false;
    }
    if (match_count > 1) {
        error = "multiple output tensor sets match TensorMap";
        return false;
    }
    error.clear();
    return true;
}

} // namespace

bool validate_labels(const std::vector<std::string>& labels, std::string& error)
{
    if (labels.empty()) {
        error = "labels must not be empty";
        return false;
    }
    std::unordered_set<std::string> unique;
    unique.reserve(labels.size());
    for (const auto& label : labels) {
        if (label.empty()) {
            error = "labels must not contain empty strings";
            return false;
        }
        if (!unique.emplace(label).second) {
            error = "labels must be unique";
            return false;
        }
    }
    error.clear();
    return true;
}

bool resolve_model(const ModelFacts& facts, const json::value* descriptor_json, ResolvedModel& output, std::string& error)
{
    DescriptorOverrides overrides;
    if (descriptor_json && !parse_descriptor(*descriptor_json, overrides, error)) {
        return false;
    }

    const MetadataProfile profile = read_metadata_profile(facts);
    ModelDescriptor descriptor;
    if (!resolve_descriptor(overrides, profile, descriptor, error)) {
        return false;
    }

    const size_t expected_input_count = descriptor.input.original_size ? 2 : 1;
    if (facts.inputs.size() != expected_input_count) {
        error = "detector input count does not match the resolved input steps";
        return false;
    }
    std::vector<const TensorSpec*> image_inputs;
    for (const auto& input : facts.inputs) {
        if (is_image_input(input, descriptor.input.layout)) {
            image_inputs.emplace_back(&input);
        }
    }
    if (image_inputs.size() != 1) {
        error = image_inputs.empty() ? "detector image input was not found" : "multiple detector image inputs match the resolved layout";
        return false;
    }
    const TensorSpec& image_input = *image_inputs.front();
    if (!validate_image_input(descriptor, image_input, error)) {
        error = "detector input contract mismatch: " + error;
        return false;
    }

    std::vector<std::string> input_names = { descriptor.input.name };
    if (descriptor.input.original_size) {
        std::vector<const TensorSpec*> original_size_inputs;
        for (const auto& input : facts.inputs) {
            if (&input != &image_input && is_original_size_input(input)) {
                original_size_inputs.emplace_back(&input);
            }
        }
        if (original_size_inputs.size() != 1) {
            error = original_size_inputs.empty() ? "OriginalSize auxiliary input was not found" : "multiple inputs match OriginalSize";
            return false;
        }
        descriptor.input.original_size->name = original_size_inputs.front()->name;
        input_names.emplace_back(descriptor.input.original_size->name);
    }

    auto adapter = create(descriptor.output.adapter);
    std::vector<TensorSpec> outputs;
    if (!adapter || !select_outputs(descriptor, *adapter, facts.outputs, outputs, error)) {
        if (error.empty()) {
            error = "failed to create detector output adapter";
        }
        else {
            error = "detector output contract mismatch: " + error;
        }
        return false;
    }

    auto resolved_metadata_labels = metadata_labels(facts);
    if (!resolved_metadata_labels.empty() && !descriptor.labels.empty() && resolved_metadata_labels != descriptor.labels) {
        LogWarn << "Ignoring conflicting model JSON labels because ONNX metadata has priority";
    }
    output = ResolvedModel {
        .descriptor = std::move(descriptor),
        .input_names = std::move(input_names),
        .outputs = std::move(outputs),
        .metadata_labels = std::move(resolved_metadata_labels),
    };
    error.clear();
    return true;
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
