#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

enum class InferenceEngine
{
    ONNXRuntime,
    NCNN,
};

struct InferenceOutput
{
    // aligned with onnxruntime, e.g. { 1, 25, 18900 }
    std::vector<int64_t> shape;
    // contiguous, row-major
    std::vector<float> data;
};

// Single input (input 0) / single output (output 0) inference session, the only dependency of the Vision layer.
class InferenceSession
{
public:
    virtual ~InferenceSession() = default;

    virtual InferenceEngine engine() const = 0;

    // NCHW shape of input 0
    virtual const std::vector<int64_t>& input_shape() const = 0;

    // input: NCHW float tensor, size must match input_shape()
    virtual std::optional<InferenceOutput> run(const std::vector<float>& input) const = 0;

    // equivalent to onnx custom metadata map, e.g. "names"
    virtual const std::unordered_map<std::string, std::string>& metadata() const = 0;
};

using InferenceSessionPtr = std::shared_ptr<InferenceSession>;

MAA_VISION_NS_END
