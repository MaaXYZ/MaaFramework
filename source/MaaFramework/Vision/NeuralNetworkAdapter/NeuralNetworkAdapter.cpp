#include "NeuralNetworkAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

std::unique_ptr<IAdapter> create(const AdapterOptions& options)
{
    switch (options.protocol) {
    case OutputProtocol::YoloDense:
        return create_yolo_dense(options);
    case OutputProtocol::YoloEndToEnd:
        return create_yolo_end_to_end();
    case OutputProtocol::DetrQueries:
        return create_detr_queries(options);
    case OutputProtocol::DetrPacked:
        return create_detr_packed(options);
    }
    return nullptr;
}

bool has_batch_one(const std::vector<int64_t>& shape)
{
    return !shape.empty() && (shape.front() == 1 || shape.front() == -1);
}

bool check_finite(std::span<const float> values, std::string_view protocol, std::string& error)
{
    if (std::ranges::all_of(values, [](float value) { return std::isfinite(value); })) {
        return true;
    }
    error = std::string(protocol) + " output contains non-finite values";
    return false;
}

bool decode_class_id(float value, int& class_id, std::string_view protocol, std::string& error)
{
    const float rounded = std::round(value);
    if (!std::isfinite(value) || value < 0.0F || std::abs(value - rounded) > 1.0e-5F
        || rounded > static_cast<float>(std::numeric_limits<int>::max())) {
        error = std::string(protocol) + " class id must be a non-negative integer";
        return false;
    }
    class_id = static_cast<int>(rounded);
    return true;
}

bool validate_score(float value, float& score, std::string_view protocol, std::string& error)
{
    constexpr float kTolerance = 1.0e-5F;
    if (!std::isfinite(value) || value < -kTolerance || value > 1.0F + kTolerance) {
        error = std::string(protocol) + " score is outside [0, 1]";
        return false;
    }
    score = std::clamp(value, 0.0F, 1.0F);
    return true;
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
