#include "NeuralNetworkAdapter.h"

#include <algorithm>
#include <limits>

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

class YoloDenseAdapter final : public IAdapter
{
public:
    explicit YoloDenseAdapter(const AdapterOptions& options)
        : layout_(options.yolo_dense_layout)
        , multi_label_(options.yolo_dense_multi_label)
    {
    }

    bool validate(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
            error = "YoloDense requires exactly one FP32 output";
            return false;
        }
        const auto& shape = tensors.front().shape;
        if (shape.size() != 3 || !has_batch_one(shape)) {
            error = "YoloDense output must have rank three and batch one";
            return false;
        }
        const int64_t feature_count = layout_ == YoloDenseLayout::ChannelsFirst ? shape[1] : shape[2];
        const int64_t proposal_count = layout_ == YoloDenseLayout::ChannelsFirst ? shape[2] : shape[1];
        if (feature_count <= 4 || proposal_count == 0 || proposal_count < -1) {
            error = "YoloDense output must have shape [1, 4 + C, N] or [1, N, 4 + C]";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
        const override
    {
        if (tensors.size() != 1 || tensors.front().element_type != TensorElementType::Float32) {
            error = "YoloDense requires exactly one FP32 output";
            return false;
        }
        const auto& tensor = tensors.front();
        if (tensor.shape.size() != 3 || tensor.shape[0] != 1 || tensor.shape[1] <= 0 || tensor.shape[2] <= 0) {
            error = "YoloDense output must have a concrete batch-one rank-three shape";
            return false;
        }

        const size_t dim1 = static_cast<size_t>(tensor.shape[1]);
        const size_t dim2 = static_cast<size_t>(tensor.shape[2]);
        const size_t feature_count = layout_ == YoloDenseLayout::ChannelsFirst ? dim1 : dim2;
        const size_t proposal_count = layout_ == YoloDenseLayout::ChannelsFirst ? dim2 : dim1;
        if (feature_count <= 4 || tensor.float_data.size() != feature_count * proposal_count) {
            error = "YoloDense output has invalid dimensions";
            return false;
        }
        if (!check_finite(tensor.float_data, "YoloDense", error)) {
            return false;
        }

        const size_t class_count = feature_count - 4;
        if (class_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
            error = "YoloDense class count exceeds the supported range";
            return false;
        }
        const auto at = [&](size_t proposal, size_t feature) {
            return layout_ == YoloDenseLayout::ChannelsFirst ? tensor.float_data[feature * proposal_count + proposal]
                                                             : tensor.float_data[proposal * feature_count + feature];
        };

        output = { };
        output.class_count = class_count;
        output.candidates.reserve(proposal_count);
        for (size_t proposal = 0; proposal < proposal_count; ++proposal) {
            const std::array<float, 4> box = { at(proposal, 0), at(proposal, 1), at(proposal, 2), at(proposal, 3) };
            if (box[2] < 0.0F || box[3] < 0.0F) {
                error = "YoloDense output contains a negative box size";
                return false;
            }

            if (multi_label_) {
                for (size_t class_id = 0; class_id < class_count; ++class_id) {
                    float score = 0.0F;
                    if (!validate_score(at(proposal, class_id + 4), score, "YoloDense", error)) {
                        return false;
                    }
                    if (score >= options.score_floor) {
                        output.candidates.emplace_back(RawDetection { .box = box, .score = score, .class_id = static_cast<int>(class_id) });
                    }
                }
                continue;
            }

            size_t best_class = 0;
            float best_score = -std::numeric_limits<float>::infinity();
            for (size_t class_id = 0; class_id < class_count; ++class_id) {
                float score = 0.0F;
                if (!validate_score(at(proposal, class_id + 4), score, "YoloDense", error)) {
                    return false;
                }
                if (score > best_score) {
                    best_score = score;
                    best_class = class_id;
                }
            }
            if (best_score >= options.score_floor) {
                output.candidates.emplace_back(RawDetection { .box = box, .score = best_score, .class_id = static_cast<int>(best_class) });
            }
        }
        error.clear();
        return true;
    }

private:
    YoloDenseLayout layout_;
    bool multi_label_ = false;
};

} // namespace

std::unique_ptr<IAdapter> create_yolo_dense(const AdapterOptions& options)
{
    return std::make_unique<YoloDenseAdapter>(options);
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
