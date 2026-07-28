#include "NeuralNetworkAdapter.h"

#include <limits>

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

bool compatible_count(int64_t lhs, int64_t rhs)
{
    return lhs <= 0 || rhs <= 0 || lhs == rhs;
}

class DetrPackedAdapter final : public IAdapter
{
public:
    explicit DetrPackedAdapter(DetrPackedLayout layout)
        : layout_(layout)
    {
    }

    bool validate(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (layout_ == DetrPackedLayout::Interleaved) {
            if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
                error = "DetrPacked Interleaved requires exactly one FP32 output";
                return false;
            }
            const auto& shape = tensors.front().shape;
            if (shape.size() != 3 || !has_batch_one(shape) || shape[1] == 0 || shape[1] < -1 || shape[2] != 6) {
                error = "DetrPacked Interleaved output must have shape [1, K, 6]";
                return false;
            }
            error.clear();
            return true;
        }

        if (tensors.size() != 3 || tensors[0].element_type != TensorElementType::Int64
            || tensors[1].element_type != TensorElementType::Float32 || tensors[2].element_type != TensorElementType::Float32) {
            error = "DetrPacked Split requires INT64 labels, FP32 boxes, and FP32 scores";
            return false;
        }
        const auto& labels = tensors[0].shape;
        const auto& boxes = tensors[1].shape;
        const auto& scores = tensors[2].shape;
        if (labels.size() != 2 || !has_batch_one(labels) || labels[1] == 0 || labels[1] < -1 || boxes.size() != 3 || !has_batch_one(boxes)
            || boxes[1] == 0 || boxes[1] < -1 || boxes[2] != 4 || scores.size() != 2 || !has_batch_one(scores) || scores[1] == 0
            || scores[1] < -1 || !compatible_count(labels[1], boxes[1]) || !compatible_count(labels[1], scores[1])) {
            error = "DetrPacked Split outputs must have shapes [1, K], [1, K, 4], and [1, K]";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
        const override
    {
        if (layout_ == DetrPackedLayout::Interleaved) {
            return decode_interleaved(tensors, options, output, error);
        }
        return decode_split(tensors, options, output, error);
    }

private:
    static bool
        decode_interleaved(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
    {
        if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
            error = "DetrPacked Interleaved requires exactly one FP32 output";
            return false;
        }
        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] < 0 || tensor.shape[2] != 6
            || tensor.float_data.size() != static_cast<size_t>(tensor.shape[1]) * 6) {
            error = "DetrPacked Interleaved output must have concrete shape [1, K, 6]";
            return false;
        }
        if (!check_finite(tensor.float_data, "DetrPacked Interleaved", error)) {
            return false;
        }

        output = { };
        output.candidates.reserve(static_cast<size_t>(tensor.shape[1]));
        for (size_t index = 0; index < static_cast<size_t>(tensor.shape[1]); ++index) {
            const auto row = tensor.float_data.subspan(index * 6, 6);
            float score = 0.0F;
            int class_id = -1;
            if (!validate_score(row[4], score, "DetrPacked Interleaved", error)
                || !decode_class_id(row[5], class_id, "DetrPacked Interleaved", error)) {
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

    static bool
        decode_split(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
    {
        if (tensors.size() != 3 || tensors[0].element_type != TensorElementType::Int64
            || tensors[1].element_type != TensorElementType::Float32 || tensors[2].element_type != TensorElementType::Float32) {
            error = "DetrPacked Split requires INT64 labels, FP32 boxes, and FP32 scores";
            return false;
        }
        const auto& labels = tensors[0];
        const auto& boxes = tensors[1];
        const auto& scores = tensors[2];
        if (labels.shape.size() != 2 || labels.shape[0] != 1 || labels.shape[1] < 0 || boxes.shape.size() != 3 || boxes.shape[0] != 1
            || boxes.shape[1] != labels.shape[1] || boxes.shape[2] != 4 || scores.shape.size() != 2 || scores.shape[0] != 1
            || scores.shape[1] != labels.shape[1]) {
            error = "DetrPacked Split outputs have invalid runtime shapes";
            return false;
        }

        const size_t count = static_cast<size_t>(labels.shape[1]);
        if (labels.int64_data.size() != count || boxes.float_data.size() != count * 4 || scores.float_data.size() != count) {
            error = "DetrPacked Split outputs have invalid dimensions";
            return false;
        }
        if (!check_finite(boxes.float_data, "DetrPacked Split", error) || !check_finite(scores.float_data, "DetrPacked Split", error)) {
            return false;
        }

        output = { };
        output.candidates.reserve(count);
        for (size_t index = 0; index < count; ++index) {
            if (labels.int64_data[index] < 0 || labels.int64_data[index] > std::numeric_limits<int>::max()) {
                error = "DetrPacked Split class id is outside the supported range";
                return false;
            }
            float score = 0.0F;
            if (!validate_score(scores.float_data[index], score, "DetrPacked Split", error)) {
                return false;
            }
            if (score < options.score_floor) {
                continue;
            }
            const auto box = boxes.float_data.subspan(index * 4, 4);
            output.candidates.emplace_back(
                RawDetection {
                    .box = { box[0], box[1], box[2], box[3] },
                    .score = score,
                    .class_id = static_cast<int>(labels.int64_data[index]),
                });
        }
        error.clear();
        return true;
    }

    DetrPackedLayout layout_;
};

} // namespace

std::unique_ptr<IAdapter> create_detr_packed(const AdapterOptions& options)
{
    return std::make_unique<DetrPackedAdapter>(options.detr_packed_layout);
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
