#include "ProjectInterface/Parser.h"

#include <functional>
#include <ranges>
#include <unordered_set>

#include "MaaUtils/Logger.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

std::optional<InterfaceData> Parser::parse_interface(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path, true, true);
    if (!json_opt) {
        LogError << "failed to parse" << path;
        return std::nullopt;
    }

    const json::value& json = *json_opt;
    auto data_opt = parse_interface(json);
    if (!data_opt) {
        return std::nullopt;
    }

    InterfaceData& data = *data_opt;

    auto base_dir = path.parent_path();
    for (const std::string& import_path : data_opt->import_) {
        auto import_full_path = base_dir / MaaNS::path(import_path);
        auto import_data = parse_import_data(import_full_path);
        if (!import_data) {
            LogError << "failed to parse import interface data" << VAR(import_full_path) << VAR(import_path);
            return std::nullopt;
        }
        data.task.insert(
            data.task.end(),
            std::make_move_iterator(import_data->task.begin()),
            std::make_move_iterator(import_data->task.end()));

        data.option.insert(std::make_move_iterator(import_data->option.begin()), std::make_move_iterator(import_data->option.end()));
    }

    return data;
}

std::optional<InterfaceData> Parser::parse_interface(const json::value& json)
{
    std::string error_key;
    if (!InterfaceData().check_json(json, error_key)) {
        LogError << "json is not an InterfaceData" << VAR(error_key) << VAR(json);
        return std::nullopt;
    }

    auto data = json.as<InterfaceData>();

    // check interface version
    if (data.interface_version != 2) {
        LogError << "Unsupported interface version, expected 2" << VAR(data.interface_version);
        return std::nullopt;
    }

    // check option for task
    for (auto& task : data.task) {
        for (auto& option : task.option) {
            if (!data.option.contains(option)) {
                LogError << "Option not found" << VAR(option);
                return std::nullopt;
            }
        }
    }

    // check controller type
    for (auto& ctrl : data.controller) {
        if (ctrl.type == InterfaceData::Controller::Type::Invalid) {
            LogError << "Invalid Controller Type" << VAR(ctrl.type);
            return std::nullopt;
        }
    }

    LogInfo << "Interface Version:" << VAR(data.version);
    return data;
}

std::optional<Configuration> Parser::parse_config(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path, true, true);
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

std::optional<ImportData> Parser::parse_import_data(const std::filesystem::path& path)
{
    LogFunc << VAR(path);
    auto json_opt = json::open(path, true, true);
    if (!json_opt) {
        LogError << "failed to parse import interface" << VAR(path);
        return std::nullopt;
    }

    const json::value& json = *json_opt;
    return parse_import_data(json);
}

std::optional<ImportData> Parser::parse_import_data(const json::value& json)
{
    std::string error_key;
    if (!ImportData().check_json(json, error_key)) {
        LogError << "json is not a valid ImportData" << VAR(error_key);
        return std::nullopt;
    }

    return json.as<ImportData>();
}

bool Parser::check_configuration(const InterfaceData& data, Configuration& config)
{
    bool erased = false;

    for (auto iter = config.task.begin(); iter != config.task.end();) {
        bool checked = check_task(data, *iter);
        if (checked) {
            ++iter;
        }
        else {
            iter = config.task.erase(iter);
            erased = true;
        }
    }

    auto resource_iter = std::ranges::find(data.resource, config.resource, std::mem_fn(&InterfaceData::Resource::name));
    if (resource_iter == data.resource.end()) {
        LogWarn << "Resource not found" << VAR(config.resource);
        config.resource.clear();
        return false;
    }

    auto controller_iter = std::ranges::find(data.controller, config.controller.name, std::mem_fn(&InterfaceData::Controller::name));
    if (controller_iter == data.controller.end()) {
        LogWarn << "Controller not found" << VAR(config.controller.name);
        config.controller.name.clear();
        return false;
    }
    config.controller.type = controller_iter->type;

    return !erased;
}

bool Parser::check_task(const InterfaceData& data, Configuration::Task& config_task)
{
    auto data_iter = std::ranges::find(data.task, config_task.name, std::mem_fn(&InterfaceData::Task::name));
    if (data_iter == data.task.end()) {
        LogWarn << "Task not found" << VAR(config_task.name);
        return false;
    }

    for (auto& config_option : config_task.option) {
        auto option_iter = data.option.find(config_option.name);
        if (option_iter == data.option.end()) {
            LogWarn << "Option not found" << VAR(config_task.name) << VAR(config_option.name);
            return false;
        }

        const InterfaceData::Option& data_option = option_iter->second;

        switch (data_option.type) {
        case InterfaceData::Option::Type::Select:
        case InterfaceData::Option::Type::Switch: {
            auto case_iter = std::ranges::find(data_option.cases, config_option.value, std::mem_fn(&InterfaceData::Option::Case::name));
            if (case_iter == data_option.cases.end()) {
                LogWarn << "Case not found" << VAR(config_task.name) << VAR(config_option.name) << VAR(config_option.value);
                return false;
            }
        } break;
        case InterfaceData::Option::Type::Input:
            // input type uses inputs map, no case validation needed
            break;
        }
    }

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
