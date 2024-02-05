#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <utility>

#include "Types.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Parser
{
public:
    static std::optional<InterfaceData> parse(const std::filesystem::path& path);
};

MAA_PROJECT_INTERFACE_NS_END
