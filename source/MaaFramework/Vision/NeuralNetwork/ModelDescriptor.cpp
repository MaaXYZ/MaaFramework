#include "ModelDescriptor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

#include "LabelParser.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

namespace
{

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

bool parse_finite_float(const json::value& object, std::string_view key, float& output, std::string& error)
{
    const auto number = object.find<double>(std::string(key));
    if (!number || !std::isfinite(*number)) {
        error = std::string(key) + " must be a finite number";
        return false;
    }
    output = static_cast<float>(*number);
    if (!std::isfinite(output)) {
        error = std::string(key) + " is outside the supported float range";
        return false;
    }
    return true;
}

bool parse_triplet(const json::value& value, std::array<float, 3>& output, std::string_view name, std::string& error)
{
    if (!value.is_array() || value.as_array().size() != output.size()) {
        error = std::string(name) + " must be an array of three finite numbers";
        return false;
    }
    for (size_t index = 0; index < output.size(); ++index) {
        const auto& item = value.as_array()[index];
        if (!item.is_number() || !std::isfinite(item.as_double())) {
            error = std::string(name) + " must be an array of three finite numbers";
            return false;
        }
        output[index] = static_cast<float>(item.as_double());
        if (!std::isfinite(output[index])) {
            error = std::string(name) + " contains a number outside the float range";
            return false;
        }
    }
    return true;
}

bool parse_input(const json::value& input, ModelDescriptor& descriptor, std::string& error)
{
    if (!input.is_object()) {
        error = "input must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowedFields = { "name", "size" };
    if (!check_allowed_fields(input.as_object(), kAllowedFields, "input", error)) {
        return false;
    }

    if (auto name = input.find<std::string>("name")) {
        if (name->empty()) {
            error = "input.name must not be empty";
            return false;
        }
        descriptor.input_name = *name;
    }
    else if (input.exists("name")) {
        error = "input.name must be a string";
        return false;
    }

    if (auto size = input.find("size")) {
        if (!size->is_array() || size->as_array().size() != 2) {
            error = "input.size must be [width, height]";
            return false;
        }
        const auto width = size->as_array()[0].is_number() ? size->as_array()[0].as_double() : -1.0;
        const auto height = size->as_array()[1].is_number() ? size->as_array()[1].as_double() : -1.0;
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0 || std::trunc(width) != width
            || std::trunc(height) != height || width > static_cast<double>(std::numeric_limits<int>::max())
            || height > static_cast<double>(std::numeric_limits<int>::max())) {
            error = "input.size must contain two positive integers";
            return false;
        }
        descriptor.preprocess.input_size = { static_cast<int>(width), static_cast<int>(height) };
    }
    return true;
}

bool parse_preprocess(const json::value& input, PreprocessSpec& spec, std::string& error)
{
    if (!input.is_object()) {
        error = "preprocess must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowedFields = {
        "resize", "interpolation", "color", "scale", "mean", "std", "padding_value",
    };
    if (!check_allowed_fields(input.as_object(), kAllowedFields, "preprocess", error)) {
        return false;
    }

    if (auto resize = input.find<std::string>("resize")) {
        if (*resize == "Stretch") {
            spec.resize_mode = ResizeMode::Stretch;
        }
        else if (*resize == "Letterbox") {
            spec.resize_mode = ResizeMode::Letterbox;
        }
        else {
            error = "preprocess.resize must be Stretch or Letterbox";
            return false;
        }
    }
    else if (input.exists("resize")) {
        error = "preprocess.resize must be a string";
        return false;
    }

    if (auto interpolation = input.find<std::string>("interpolation")) {
        if (*interpolation == "Nearest") {
            spec.interpolation = cv::INTER_NEAREST;
        }
        else if (*interpolation == "Linear") {
            spec.interpolation = cv::INTER_LINEAR;
        }
        else if (*interpolation == "Cubic") {
            spec.interpolation = cv::INTER_CUBIC;
        }
        else if (*interpolation == "Area") {
            spec.interpolation = cv::INTER_AREA;
        }
        else {
            error = "preprocess.interpolation is not supported";
            return false;
        }
    }
    else if (input.exists("interpolation")) {
        error = "preprocess.interpolation must be a string";
        return false;
    }

    if (auto color = input.find<std::string>("color")) {
        if (*color == "RGB") {
            spec.color_order = ColorOrder::RGB;
        }
        else if (*color == "BGR") {
            spec.color_order = ColorOrder::BGR;
        }
        else {
            error = "preprocess.color must be RGB or BGR";
            return false;
        }
    }
    else if (input.exists("color")) {
        error = "preprocess.color must be a string";
        return false;
    }

    if (input.exists("scale") && !parse_finite_float(input, "scale", spec.scale, error)) {
        return false;
    }
    if (auto mean = input.find("mean"); mean && !parse_triplet(*mean, spec.mean, "preprocess.mean", error)) {
        return false;
    }
    if (auto std = input.find("std"); std && !parse_triplet(*std, spec.std, "preprocess.std", error)) {
        return false;
    }
    if (std::ranges::any_of(spec.std, [](float value) { return value == 0.0F; })) {
        error = "preprocess.std must not contain zero";
        return false;
    }
    if (auto padding = input.find("padding_value");
        padding && !parse_triplet(*padding, spec.padding_value, "preprocess.padding_value", error)) {
        return false;
    }
    return true;
}

bool parse_nms(std::string_view value, NmsPolicy& output)
{
    if (value == "None") {
        output = NmsPolicy::None;
        return true;
    }
    if (value == "ClassAwareIoU") {
        output = NmsPolicy::ClassAwareIoU;
        return true;
    }
    if (value == "CandidateCoverage") {
        output = NmsPolicy::CandidateCoverage;
        return true;
    }
    return false;
}

} // namespace

bool ModelDescriptorParser::parse(
    const json::value& input,
    const ModelAdapterRegistry& registry,
    ModelDescriptor& descriptor,
    std::unique_ptr<IModelAdapter>& adapter,
    std::string& error)
{
    if (!input.is_object()) {
        error = "model descriptor must be an object";
        return false;
    }
    static const std::unordered_set<std::string_view> kAllowedFields = {
        "$schema", "adapter",    "producer", "producer_version", "export_args",    "labels",
        "input",   "preprocess", "nms",      "nms_threshold",    "adapter_config",
    };
    if (!check_allowed_fields(input.as_object(), kAllowedFields, "model descriptor", error)) {
        return false;
    }

    const auto adapter_id = input.find<std::string>("adapter");
    if (!adapter_id || adapter_id->empty()) {
        error = "adapter must be a non-empty string";
        return false;
    }
    if (input.exists("$schema") && !input.find<std::string>("$schema")) {
        error = "$schema must be a string";
        return false;
    }

    auto parsed_adapter = registry.create(*adapter_id);
    if (!parsed_adapter) {
        error = "adapter is not registered: " + *adapter_id;
        return false;
    }

    ModelDescriptor parsed;
    parsed.adapter = *adapter_id;
    parsed.preprocess = parsed_adapter->default_preprocess();
    parsed.nms = parsed_adapter->default_nms();
    parsed.nms_threshold = parsed_adapter->default_nms_threshold();

    for (const std::string& name : { std::string("producer"), std::string("producer_version") }) {
        if (input.exists(name) && !input.find<std::string>(name)) {
            error = name + " must be a string";
            return false;
        }
    }
    if (auto export_args = input.find("export_args"); export_args && !export_args->is_object()) {
        error = "export_args must be an object";
        return false;
    }

    if (auto labels = input.find("labels"); labels && !LabelParser::parse(*labels, parsed.labels, error)) {
        return false;
    }
    if (auto input_spec = input.find("input"); input_spec && !parse_input(*input_spec, parsed, error)) {
        return false;
    }
    if (auto preprocess = input.find("preprocess"); preprocess && !parse_preprocess(*preprocess, parsed.preprocess, error)) {
        return false;
    }

    if (auto nms = input.find<std::string>("nms")) {
        if (!parse_nms(*nms, parsed.nms)) {
            error = "nms is not a supported policy";
            return false;
        }
    }
    else if (input.exists("nms")) {
        error = "nms must be a string";
        return false;
    }
    if (input.exists("nms_threshold")) {
        if (!parse_finite_float(input, "nms_threshold", parsed.nms_threshold, error) || parsed.nms_threshold < 0.0F
            || parsed.nms_threshold > 1.0F) {
            error = "nms_threshold must be a finite number in [0, 1]";
            return false;
        }
    }

    json::object adapter_config;
    if (auto config = input.find("adapter_config")) {
        if (!config->is_object()) {
            error = "adapter_config must be an object";
            return false;
        }
        adapter_config = config->as_object();
    }
    if (!parsed_adapter->configure(adapter_config, error)) {
        return false;
    }

    descriptor = std::move(parsed);
    adapter = std::move(parsed_adapter);
    error.clear();
    return true;
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
