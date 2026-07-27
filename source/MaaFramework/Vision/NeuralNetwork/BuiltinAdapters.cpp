#include "ModelAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

namespace
{

bool check_allowed_fields(const json::object& config, const std::unordered_set<std::string_view>& allowed, std::string& error)
{
    for (const auto& [key, value] : config) {
        (void)value;
        if (!allowed.contains(key)) {
            error = "unknown adapter_config field: " + key;
            return false;
        }
    }
    return true;
}

bool decode_class_id(float value, int& class_id, std::string_view adapter_id, std::string& error)
{
    const float rounded = std::round(value);
    if (!std::isfinite(value) || value < 0.0F || std::abs(value - rounded) > 1.0e-5F
        || rounded > static_cast<float>(std::numeric_limits<int>::max())) {
        error = std::string(adapter_id) + " class id must be a non-negative integer";
        return false;
    }

    class_id = static_cast<int>(rounded);
    return true;
}

bool check_finite_tensor(const TensorView& tensor, std::string_view adapter_id, std::string& error)
{
    if (std::ranges::all_of(tensor.data, [](float value) { return std::isfinite(value); })) {
        return true;
    }

    error = std::string(adapter_id) + " output contains non-finite values";
    return false;
}

bool has_batch_one(const std::vector<int64_t>& shape)
{
    return !shape.empty() && (shape[0] == 1 || shape[0] == -1);
}

class UltralyticsYoloDenseAdapter final : public IModelAdapter
{
public:
    std::string_view id() const override { return "ultralytics_yolo_dense"; }

    PreprocessSpec default_preprocess() const override
    {
        PreprocessSpec spec;
        spec.resize_mode = ResizeMode::Letterbox;
        spec.padding_value = { 114.0F, 114.0F, 114.0F };
        return spec;
    }

    NmsPolicy default_nms() const override { return NmsPolicy::ClassAwareIoU; }

    bool configure(const json::object& config, std::string& error) override
    {
        static const std::unordered_set<std::string_view> kAllowedFields = { "multi_label", "output_layout" };
        if (!check_allowed_fields(config, kAllowedFields, error)) {
            return false;
        }

        json::value value(config);
        if (auto multi_label = value.find<bool>("multi_label")) {
            multi_label_ = *multi_label;
        }
        else if (value.exists("multi_label")) {
            error = "adapter_config.multi_label must be boolean";
            return false;
        }

        if (auto output_layout = value.find<std::string>("output_layout")) {
            if (*output_layout == "Auto") {
                output_layout_ = OutputLayout::Auto;
            }
            else if (*output_layout == "ChannelsFirst") {
                output_layout_ = OutputLayout::ChannelsFirst;
            }
            else if (*output_layout == "ChannelsLast") {
                output_layout_ = OutputLayout::ChannelsLast;
            }
            else {
                error = "adapter_config.output_layout must be Auto, ChannelsFirst, or ChannelsLast";
                return false;
            }
        }
        else if (value.exists("output_layout")) {
            error = "adapter_config.output_layout must be a string";
            return false;
        }

        error.clear();
        return true;
    }

    bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 3 || !has_batch_one(tensors.front().shape)
            || tensors.front().shape[1] < -1 || tensors.front().shape[2] < -1 || tensors.front().shape[1] == 0
            || tensors.front().shape[2] == 0) {
            error = "ultralytics_yolo_dense output must have shape [1, 4 + C, N] or [1, N, 4 + C]";
            return false;
        }
        const auto& shape = tensors.front().shape;
        bool channels_first = false;
        if (!resolve_output_layout(shape[1], shape[2], channels_first, error)) {
            return false;
        }
        const int64_t feature_count = channels_first ? shape[1] : shape[2];
        if (feature_count <= 4) {
            error = "ultralytics_yolo_dense output feature count must be greater than four";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const override
    {
        if (tensors.size() != 1) {
            error = "ultralytics_yolo_dense expects one output tensor";
            return false;
        }

        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] <= 0 || tensor.shape[2] <= 0) {
            error = "ultralytics_yolo_dense output must have shape [1, 4 + C, N] or [1, N, 4 + C]";
            return false;
        }

        const size_t dim1 = static_cast<size_t>(tensor.shape[1]);
        const size_t dim2 = static_cast<size_t>(tensor.shape[2]);
        bool channels_first = false;
        if (!resolve_output_layout(tensor.shape[1], tensor.shape[2], channels_first, error)) {
            return false;
        }
        const size_t feature_count = channels_first ? dim1 : dim2;
        const size_t proposal_count = channels_first ? dim2 : dim1;
        if (feature_count <= 4 || tensor.data.size() != feature_count * proposal_count) {
            error = "ultralytics_yolo_dense output has invalid dimensions";
            return false;
        }
        if (!check_finite_tensor(tensor, id(), error)) {
            return false;
        }

        const size_t class_count = feature_count - 4;
        if (class_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
            error = "ultralytics_yolo_dense class count exceeds the supported range";
            return false;
        }
        auto at = [&](size_t proposal, size_t feature) -> float {
            return channels_first ? tensor.data[feature * proposal_count + proposal] : tensor.data[proposal * feature_count + feature];
        };

        output.candidates.clear();
        output.class_count = class_count;
        output.candidates.reserve(proposal_count);
        for (size_t proposal = 0; proposal < proposal_count; ++proposal) {
            const float center_x = at(proposal, 0);
            const float center_y = at(proposal, 1);
            const float width = at(proposal, 2);
            const float height = at(proposal, 3);
            if (width < 0.0F || height < 0.0F) {
                error = "ultralytics_yolo_dense output contains a negative box size";
                return false;
            }
            const cv::Rect2f input_box {
                center_x - width / 2.0F,
                center_y - height / 2.0F,
                width,
                height,
            };
            const cv::Rect2f source_box = transform.to_source(input_box);

            if (multi_label_) {
                for (size_t class_id = 0; class_id < class_count; ++class_id) {
                    float score = 0.0F;
                    if (!validate_score(at(proposal, class_id + 4), score, error)) {
                        return false;
                    }
                    if (score < options.score_floor) {
                        continue;
                    }
                    output.candidates.emplace_back(
                        DetectionCandidate {
                            .box = source_box,
                            .score = score,
                            .class_id = static_cast<int>(class_id),
                        });
                }
                continue;
            }

            size_t best_class = 0;
            float best_score = -std::numeric_limits<float>::infinity();
            for (size_t class_id = 0; class_id < class_count; ++class_id) {
                float score = 0.0F;
                if (!validate_score(at(proposal, class_id + 4), score, error)) {
                    return false;
                }
                if (score > best_score) {
                    best_score = score;
                    best_class = class_id;
                }
            }
            if (best_score < options.score_floor) {
                continue;
            }
            output.candidates.emplace_back(
                DetectionCandidate {
                    .box = source_box,
                    .score = best_score,
                    .class_id = static_cast<int>(best_class),
                });
        }

        error.clear();
        return true;
    }

private:
    enum class OutputLayout
    {
        Auto,
        ChannelsFirst,
        ChannelsLast,
    };

    bool validate_score(float input, float& score, std::string& error) const
    {
        constexpr float kScoreTolerance = 1.0e-5F;
        if (!std::isfinite(input) || input < -kScoreTolerance || input > 1.0F + kScoreTolerance) {
            error = "ultralytics_yolo_dense class score is outside [0, 1]";
            return false;
        }

        score = std::clamp(input, 0.0F, 1.0F);
        return true;
    }

    bool resolve_output_layout(int64_t dim1, int64_t dim2, bool& channels_first, std::string& error) const
    {
        if (output_layout_ == OutputLayout::ChannelsFirst) {
            channels_first = true;
            return true;
        }
        if (output_layout_ == OutputLayout::ChannelsLast) {
            channels_first = false;
            return true;
        }
        if (dim1 <= 0 || dim2 <= 0 || dim1 == dim2) {
            error =
                "ultralytics_yolo_dense Auto output layout requires unequal positive static dimensions; configure ChannelsFirst or "
                "ChannelsLast";
            return false;
        }

        channels_first = dim1 < dim2;
        return true;
    }

    bool multi_label_ = false;
    OutputLayout output_layout_ = OutputLayout::Auto;
};

class MaaYoloDenseCompatAdapter final : public IModelAdapter
{
public:
    std::string_view id() const override { return "maa_yolo_dense_compat"; }

    PreprocessSpec default_preprocess() const override
    {
        PreprocessSpec spec;
        spec.resize_mode = ResizeMode::Stretch;
        spec.interpolation = cv::INTER_AREA;
        return spec;
    }

    NmsPolicy default_nms() const override { return NmsPolicy::CandidateCoverage; }

    bool validate_inputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 4 || tensors.front().shape[0] != 1 || tensors.front().shape[1] != 3
            || tensors.front().shape[2] == 0 || tensors.front().shape[3] == 0) {
            error = "maa_yolo_dense_compat input must be FP32 NCHW with shape [1, 3, H, W]";
            return false;
        }
        error.clear();
        return true;
    }

    bool configure(const json::object& config, std::string& error) override
    {
        if (!check_allowed_fields(config, { }, error)) {
            return false;
        }
        const auto dense_config = json::parse(R"({"multi_label":true,"output_layout":"ChannelsFirst"})").value().as_object();
        return dense_.configure(dense_config, error);
    }

    bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 3 || tensors.front().shape[0] != 1 || tensors.front().shape[1] <= 4
            || tensors.front().shape[2] == 0 || (tensors.front().shape[2] > 0 && tensors.front().shape[1] > tensors.front().shape[2])) {
            error = "maa_yolo_dense_compat output must have shape [1, 4 + C, N] with static C >= 1";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 3 || tensors.front().shape[0] != 1 || tensors.front().shape[1] <= 4
            || tensors.front().shape[2] <= 0 || tensors.front().shape[1] > tensors.front().shape[2]) {
            error = "maa_yolo_dense_compat output must have shape [1, 4 + C, N] with static C >= 1";
            return false;
        }
        return dense_.decode(tensors, transform, options, output, error);
    }

private:
    UltralyticsYoloDenseAdapter dense_;
};

class UltralyticsYoloE2EAdapter final : public IModelAdapter
{
public:
    std::string_view id() const override { return "ultralytics_yolo_e2e"; }

    PreprocessSpec default_preprocess() const override
    {
        PreprocessSpec spec;
        spec.resize_mode = ResizeMode::Letterbox;
        spec.padding_value = { 114.0F, 114.0F, 114.0F };
        return spec;
    }

    NmsPolicy default_nms() const override { return NmsPolicy::None; }

    bool configure(const json::object& config, std::string& error) override
    {
        if (!check_allowed_fields(config, { }, error)) {
            return false;
        }

        error.clear();
        return true;
    }

    bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 3 || !has_batch_one(tensors.front().shape)
            || tensors.front().shape[1] == 0 || tensors.front().shape[2] != 6) {
            error = "ultralytics_yolo_e2e output must have shape [1, K, 6]";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const override
    {
        (void)options;
        if (tensors.size() != 1) {
            error = "ultralytics_yolo_e2e expects one output tensor";
            return false;
        }

        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] < 0 || tensor.shape[2] != 6) {
            error = "ultralytics_yolo_e2e output must have shape [1, K, 6]";
            return false;
        }

        const size_t result_count = static_cast<size_t>(tensor.shape[1]);
        if (tensor.data.size() != result_count * 6) {
            error = "ultralytics_yolo_e2e output has invalid dimensions";
            return false;
        }
        if (!check_finite_tensor(tensor, id(), error)) {
            return false;
        }

        output.candidates.clear();
        output.class_count.reset();
        output.candidates.reserve(result_count);
        for (size_t index = 0; index < result_count; ++index) {
            const auto row = tensor.data.subspan(index * 6, 6);
            int class_id = 0;
            if (!decode_class_id(row[5], class_id, id(), error)) {
                return false;
            }

            const cv::Rect2f input_box {
                row[0],
                row[1],
                row[2] - row[0],
                row[3] - row[1],
            };
            output.candidates.emplace_back(
                DetectionCandidate {
                    .box = transform.to_source(input_box),
                    .score = row[4],
                    .class_id = class_id,
                });
        }

        error.clear();
        return true;
    }
};

class UltralyticsRtDetrAdapter final : public IModelAdapter
{
public:
    std::string_view id() const override { return "ultralytics_rtdetr"; }

    PreprocessSpec default_preprocess() const override
    {
        PreprocessSpec spec;
        spec.resize_mode = ResizeMode::Stretch;
        return spec;
    }

    NmsPolicy default_nms() const override { return NmsPolicy::None; }

    bool configure(const json::object& config, std::string& error) override
    {
        if (!check_allowed_fields(config, { }, error)) {
            return false;
        }

        error.clear();
        return true;
    }

    bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().shape.size() != 3 || !has_batch_one(tensors.front().shape)
            || tensors.front().shape[1] == 0 || tensors.front().shape[2] != 6) {
            error = "ultralytics_rtdetr output must have shape [1, K, 6]";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const override
    {
        (void)options;
        if (tensors.size() != 1) {
            error = "ultralytics_rtdetr expects one output tensor";
            return false;
        }

        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] < 0 || tensor.shape[2] != 6) {
            error = "ultralytics_rtdetr output must have shape [1, K, 6]";
            return false;
        }

        const size_t result_count = static_cast<size_t>(tensor.shape[1]);
        if (tensor.data.size() != result_count * 6) {
            error = "ultralytics_rtdetr output has invalid dimensions";
            return false;
        }
        if (!check_finite_tensor(tensor, id(), error)) {
            return false;
        }

        output.candidates.clear();
        output.class_count.reset();
        output.candidates.reserve(result_count);
        for (size_t index = 0; index < result_count; ++index) {
            const auto row = tensor.data.subspan(index * 6, 6);
            int class_id = 0;
            if (!decode_class_id(row[5], class_id, id(), error)) {
                return false;
            }

            const float center_x = row[0] * static_cast<float>(transform.input_size.width);
            const float center_y = row[1] * static_cast<float>(transform.input_size.height);
            const float width = row[2] * static_cast<float>(transform.input_size.width);
            const float height = row[3] * static_cast<float>(transform.input_size.height);
            const cv::Rect2f input_box {
                center_x - width / 2.0F,
                center_y - height / 2.0F,
                width,
                height,
            };
            output.candidates.emplace_back(
                DetectionCandidate {
                    .box = transform.to_source(input_box),
                    .score = row[4],
                    .class_id = class_id,
                });
        }

        error.clear();
        return true;
    }
};

class RoboflowRfDetrAdapter final : public IModelAdapter
{
public:
    std::string_view id() const override { return "roboflow_rfdetr"; }

    PreprocessSpec default_preprocess() const override
    {
        PreprocessSpec spec;
        spec.resize_mode = ResizeMode::Stretch;
        spec.mean = { 0.485F, 0.456F, 0.406F };
        spec.std = { 0.229F, 0.224F, 0.225F };
        return spec;
    }

    NmsPolicy default_nms() const override { return NmsPolicy::None; }

    std::vector<std::string> requested_output_names() const override { return { boxes_name_, logits_name_ }; }

    bool configure(const json::object& config, std::string& error) override
    {
        static const std::unordered_set<std::string_view> kAllowedFields = { "outputs" };
        if (!check_allowed_fields(config, kAllowedFields, error)) {
            return false;
        }

        json::value value(config);
        const auto outputs = value.find("outputs");
        if (!outputs || !outputs->is_object()) {
            error = "adapter_config.outputs must be an object";
            return false;
        }

        static const std::unordered_set<std::string_view> kAllowedOutputFields = { "boxes", "logits" };
        if (!check_allowed_fields(outputs->as_object(), kAllowedOutputFields, error)) {
            return false;
        }

        const auto boxes_name = outputs->find<std::string>("boxes");
        const auto logits_name = outputs->find<std::string>("logits");
        if (!boxes_name || boxes_name->empty() || !logits_name || logits_name->empty()) {
            error = "adapter_config.outputs requires non-empty boxes and logits names";
            return false;
        }
        if (*boxes_name == *logits_name) {
            error = "adapter_config.outputs boxes and logits names must differ";
            return false;
        }

        boxes_name_ = *boxes_name;
        logits_name_ = *logits_name;
        error.clear();
        return true;
    }

    bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        const TensorSpec* boxes = nullptr;
        const TensorSpec* logits = nullptr;
        for (const auto& tensor : tensors) {
            if (tensor.name == boxes_name_) {
                boxes = &tensor;
            }
            else if (tensor.name == logits_name_) {
                logits = &tensor;
            }
        }
        if (tensors.size() != 2 || !boxes || !logits) {
            error = "roboflow_rfdetr requires the configured boxes and logits outputs";
            return false;
        }
        if (boxes->shape.size() != 3 || !has_batch_one(boxes->shape) || boxes->shape[1] == 0 || boxes->shape[2] != 4) {
            error = "roboflow_rfdetr boxes output must have shape [1, Q, 4]";
            return false;
        }
        if (logits->shape.size() != 3 || !has_batch_one(logits->shape) || logits->shape[1] == 0
            || (logits->shape[2] != -1 && logits->shape[2] < 2)) {
            error = "roboflow_rfdetr logits output must have shape [1, Q, C + 1]";
            return false;
        }
        if (boxes->shape[1] > 0 && logits->shape[1] > 0 && boxes->shape[1] != logits->shape[1]) {
            error = "roboflow_rfdetr boxes and logits query counts differ";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const override
    {
        (void)options;
        const TensorView* boxes = nullptr;
        const TensorView* logits = nullptr;
        for (const auto& tensor : tensors) {
            if (tensor.name == boxes_name_) {
                boxes = &tensor;
            }
            else if (tensor.name == logits_name_) {
                logits = &tensor;
            }
        }
        if (!boxes || !logits) {
            error = "roboflow_rfdetr configured output tensor was not found";
            return false;
        }
        if (boxes->shape.size() != 3 || boxes->shape[0] != 1 || boxes->shape[1] < 0 || boxes->shape[2] != 4) {
            error = "roboflow_rfdetr boxes output must have shape [1, Q, 4]";
            return false;
        }
        if (logits->shape.size() != 3 || logits->shape[0] != 1 || logits->shape[1] != boxes->shape[1] || logits->shape[2] < 2) {
            error = "roboflow_rfdetr logits output must have shape [1, Q, C + 1]";
            return false;
        }

        const size_t query_count = static_cast<size_t>(boxes->shape[1]);
        const size_t logits_count = static_cast<size_t>(logits->shape[2]);
        if (boxes->data.size() != query_count * 4 || logits->data.size() != query_count * logits_count) {
            error = "roboflow_rfdetr output has invalid dimensions";
            return false;
        }
        if (!std::ranges::all_of(boxes->data, [](float value) { return std::isfinite(value); })
            || !std::ranges::all_of(logits->data, [](float value) { return std::isfinite(value); })) {
            error = "roboflow_rfdetr output contains non-finite values";
            return false;
        }

        const size_t class_count = logits_count - 1;
        if (class_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
            error = "roboflow_rfdetr class count exceeds the supported range";
            return false;
        }
        output.candidates.clear();
        output.class_count = class_count;
        output.candidates.reserve(query_count);
        for (size_t query = 0; query < query_count; ++query) {
            const auto box = boxes->data.subspan(query * 4, 4);
            const auto scores = logits->data.subspan(query * logits_count, class_count);
            const auto best = std::ranges::max_element(scores);
            const size_t class_id = static_cast<size_t>(std::distance(scores.begin(), best));
            const float center_x = box[0] * static_cast<float>(transform.input_size.width);
            const float center_y = box[1] * static_cast<float>(transform.input_size.height);
            const float width = box[2] * static_cast<float>(transform.input_size.width);
            const float height = box[3] * static_cast<float>(transform.input_size.height);
            const cv::Rect2f input_box {
                center_x - width / 2.0F,
                center_y - height / 2.0F,
                width,
                height,
            };
            const float score = *best >= 0.0F ? 1.0F / (1.0F + std::exp(-*best)) : std::exp(*best) / (1.0F + std::exp(*best));
            output.candidates.emplace_back(
                DetectionCandidate {
                    .box = transform.to_source(input_box),
                    .score = score,
                    .class_id = static_cast<int>(class_id),
                });
        }

        error.clear();
        return true;
    }

private:
    std::string boxes_name_;
    std::string logits_name_;
};

} // namespace

bool register_builtin_adapters(ModelAdapterRegistry& registry)
{
    if (!registry.register_factory("ultralytics_yolo_dense", []() -> std::unique_ptr<IModelAdapter> {
            return std::make_unique<UltralyticsYoloDenseAdapter>();
        })) {
        return false;
    }
    if (!registry.register_factory("maa_yolo_dense_compat", []() -> std::unique_ptr<IModelAdapter> {
            return std::make_unique<MaaYoloDenseCompatAdapter>();
        })) {
        return false;
    }

    if (!registry.register_factory("ultralytics_yolo_e2e", []() -> std::unique_ptr<IModelAdapter> {
            return std::make_unique<UltralyticsYoloE2EAdapter>();
        })) {
        return false;
    }
    if (!registry.register_factory("ultralytics_rtdetr", []() -> std::unique_ptr<IModelAdapter> {
            return std::make_unique<UltralyticsRtDetrAdapter>();
        })) {
        return false;
    }

    return registry.register_factory("roboflow_rfdetr", []() -> std::unique_ptr<IModelAdapter> {
        return std::make_unique<RoboflowRfDetrAdapter>();
    });
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
