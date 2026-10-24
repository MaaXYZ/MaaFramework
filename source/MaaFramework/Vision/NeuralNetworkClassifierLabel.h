#pragma once

#include <cstddef>
#include <format>
#include <string>
#include <vector>

namespace MaaNS::VisionNS
{

inline std::string nn_classifier_label(const std::vector<std::string>& labels, std::size_t index)
{
    return index < labels.size() ? labels[index] : std::format("Unknown_{}", index);
}

}
