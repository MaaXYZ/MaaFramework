#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <utility>

#include "Types.h"

#include "Common/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Parser
{
public:
    static std::optional<InterfaceData> parse_interface(const std::filesystem::path& path);
    static std::optional<InterfaceData> parse_interface(const json::value& json);
    static std::optional<Configuration> parse_config(const std::filesystem::path& path);
    static std::optional<Configuration> parse_config(const json::value& json);

    static bool check_configuration(const InterfaceData& data, Configuration& config);

    // 解析导入文件（仅包含 task、option 和 import 字段）
    static std::optional<InterfaceData::ImportData> parse_import_data(const json::value& json);

private:
    static bool check_task(const InterfaceData& data, Configuration::Task& config_task);
};

MAA_PROJECT_INTERFACE_NS_END
