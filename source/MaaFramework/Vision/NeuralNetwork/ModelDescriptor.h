#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ModelAdapter.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

struct ModelDescriptor
{
    std::string adapter;
    std::vector<std::string> labels;
    std::string input_name;
    PreprocessSpec preprocess;
    NmsPolicy nms = NmsPolicy::ClassAwareIoU;
    float nms_threshold = 0.7F;
};

class ModelDescriptorParser
{
public:
    static bool parse(
        const json::value& input,
        const ModelAdapterRegistry& registry,
        ModelDescriptor& descriptor,
        std::unique_ptr<IModelAdapter>& adapter,
        std::string& error);
};

} // namespace NeuralNetwork

MAA_VISION_NS_END
