#include "ProjectInterface/Parser.h"

#include "Utils/Logger.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

std::optional<InterfaceData> Parser::parse_interface(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "failed to parse" << path;
        return std::nullopt;
    }

    const json::value& json = *json_opt;
    return parse_interface(json);
}

std::optional<InterfaceData> Parser::parse_interface(const json::value& json)
{
    std::string error_key;
    if (!InterfaceData().check_json(json, error_key)) {
        LogError << "json is not an InterfaceData" << VAR(error_key) << VAR(json);
        return std::nullopt;
    }

    auto data = json.as<InterfaceData>();

    // check option for task
    for (auto& task : data.task) {
        for (auto& option : task.option) {
            if (!data.option.contains(option)) {
                LogError << "Option not found" << VAR(option);
                return std::nullopt;
            }
        }
    }

    return data;
}

std::optional<Configuration> Parser::parse_config(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogWarn << "failed to parse" << path;
        return std::nullopt;
    }

    const json::value& json = *json_opt;
    return parse_config(json);
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

bool Parser::check_configuration(const InterfaceData& data, Configuration& config)
{
    // TODO
    std::ignore = data;
    std::ignore = config;

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
