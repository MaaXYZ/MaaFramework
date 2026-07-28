#include "NeuralNetworkAdapter.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

class YoloEndToEndAdapter final : public IAdapter
{
public:
    bool validate(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
            error = "YoloEndToEnd requires exactly one FP32 output";
            return false;
        }
        const auto& shape = tensors.front().shape;
        if (shape.size() != 3 || !has_batch_one(shape) || shape[1] == 0 || shape[1] < -1 || shape[2] != 6) {
            error = "YoloEndToEnd output must have shape [1, K, 6]";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
        const override
    {
        if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
            error = "YoloEndToEnd requires exactly one FP32 output";
            return false;
        }
        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] < 0 || tensor.shape[2] != 6
            || tensor.float_data.size() != static_cast<size_t>(tensor.shape[1]) * 6) {
            error = "YoloEndToEnd output must have concrete shape [1, K, 6]";
            return false;
        }
        if (!check_finite(tensor.float_data, "YoloEndToEnd", error)) {
            return false;
        }

        output = { };
        output.candidates.reserve(static_cast<size_t>(tensor.shape[1]));
        for (size_t index = 0; index < static_cast<size_t>(tensor.shape[1]); ++index) {
            const auto row = tensor.float_data.subspan(index * 6, 6);
            float score = 0.0F;
            int class_id = -1;
            if (!validate_score(row[4], score, "YoloEndToEnd", error) || !decode_class_id(row[5], class_id, "YoloEndToEnd", error)) {
                return false;
            }
            if (score >= options.score_floor) {
                output.candidates.emplace_back(
                    RawDetection { .box = { row[0], row[1], row[2], row[3] }, .score = score, .class_id = class_id });
            }
        }
        error.clear();
        return true;
    }
};

} // namespace

std::unique_ptr<IAdapter> create_yolo_end_to_end()
{
    return std::make_unique<YoloEndToEndAdapter>();
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
