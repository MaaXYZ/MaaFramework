#include "ProjectInterface/Parser.h"

#include "Utils/Logger.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

std::optional<InterfaceData> Parser::parse_interface(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto root_opt = json::open(path);
    if (!root_opt) {
        LogError << "failed to parse" << path;
        return std::nullopt;
    }
    const json::value& root = *root_opt;

    std::string error_key;
    if (!InterfaceData().check_json(root, error_key)) {
        LogError << "json is not an InterfaceData" << VAR(error_key) << VAR(root);
        return std::nullopt;
    }

    return root.as<InterfaceData>();
}

std::optional<Configuration> Parser::parse_config(const json::value& json)
{
    LogFunc << VAR(json);

    std::string error_key;
    if (!Configuration().check_json(json, error_key)) {
        LogError << "json is not a Configuration" << VAR(error_key) << VAR(json);
        return std::nullopt;
    }

    return json.as<Configuration>();
}

MAA_PROJECT_INTERFACE_NS_END
