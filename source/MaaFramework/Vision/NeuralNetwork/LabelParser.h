#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

class LabelParser
{
public:
    static bool parse_metadata(std::string_view text, std::vector<std::string>& labels, std::string& error);
    static bool parse(const json::value& value, std::vector<std::string>& labels, std::string& error);
    static bool validate(std::vector<std::string> labels, std::vector<std::string>& output, std::string& error);
    static bool resolve(
        const std::vector<std::string>& pipeline,
        const std::vector<std::string>& descriptor,
        const std::vector<std::string>& metadata,
        std::vector<std::string>& output,
        std::string& error);
    static bool label_for(int class_id, const std::vector<std::string>& labels, std::string& output, std::string& error);
};

} // namespace NeuralNetwork

MAA_VISION_NS_END
