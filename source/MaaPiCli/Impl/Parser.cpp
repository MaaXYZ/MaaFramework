#include "ProjectInterface/Parser.h"

#include <functional>
#include <ranges>
#include <unordered_set>

#include "MaaUtils/Logger.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

namespace
{
// Forward declaration
bool parse_and_merge_imports(
    InterfaceData& data,
    const std::filesystem::path& base_dir,
    std::unordered_set<std::string>& loaded_files);

bool merge_imported_data(InterfaceData& data, const InterfaceData::ImportData& imported)
{
    // Merge tasks - append imported tasks (check for duplicates by name)
    for (const auto& imported_task : imported.task) {
        auto existing = std::ranges::find(data.task, imported_task.name, &InterfaceData::Task::name);
        if (existing == data.task.end()) {
            data.task.emplace_back(imported_task);
        }
        else {
            LogWarn << "Duplicate task name in import, skipping:" << imported_task.name;
        }
    }

    // Merge options - append imported options (check for duplicates by key)
    for (const auto& [key, imported_option] : imported.option) {
        if (!data.option.contains(key)) {
            data.option[key] = imported_option;
        }
        else {
            LogWarn << "Duplicate option key in import, skipping:" << key;
        }
    }

    return true;
}

bool parse_and_merge_imports(
    InterfaceData& data,
    const std::filesystem::path& base_dir,
    std::unordered_set<std::string>& loaded_files)
{
    for (const auto& import_path_str : data.import_) {
        auto import_path = base_dir / import_path_str;
        auto canonical_path = std::filesystem::weakly_canonical(import_path).string();

        // Check for circular imports
        if (loaded_files.contains(canonical_path)) {
            LogWarn << "Circular import detected, skipping:" << import_path;
            continue;
        }
        loaded_files.insert(canonical_path);

        LogInfo << "Loading imported interface:" << import_path;

        auto json_opt = json::open(import_path, true, true);
        if (!json_opt) {
            LogError << "Failed to open imported interface:" << import_path;
            return false;
        }

        // Use centralized parsing for imported data
        auto imported_opt = Parser::parse_import_data(*json_opt);
        if (!imported_opt) {
            LogError << "Failed to parse imported interface:" << import_path;
            return false;
        }

        InterfaceData::ImportData& imported = *imported_opt;

        // Recursively load imports from imported file
        if (!imported.import_.empty()) {
            // Create a temporary InterfaceData to hold the recursively imported data
            InterfaceData temp_data;
            temp_data.task = std::move(imported.task);
            temp_data.option = std::move(imported.option);
            temp_data.import_ = std::move(imported.import_);

            auto import_dir = import_path.parent_path();
            if (!parse_and_merge_imports(temp_data, import_dir, loaded_files)) {
                return false;
            }

            // Move the data back
            imported.task = std::move(temp_data.task);
            imported.option = std::move(temp_data.option);
        }

        if (!merge_imported_data(data, imported)) {
            return false;
        }
    }

    return true;
}
} // namespace

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

    // Process imports if present
    if (!data_opt->import_.empty()) {
        auto base_dir = path.parent_path();
        auto canonical_path = std::filesystem::weakly_canonical(path).string();
        std::unordered_set<std::string> loaded_files { canonical_path };

        if (!parse_and_merge_imports(*data_opt, base_dir, loaded_files)) {
            LogError << "Failed to process imports for" << path;
            return std::nullopt;
        }

        // Clear import_ after processing to avoid confusing downstream consumers
        data_opt->import_.clear();
    }

    return data_opt;
}

std::optional<InterfaceData> Parser::parse_interface(const json::value& json)
{
    // 预处理：将单个 agent 对象转换为数组以支持两种格式
    json::value processed_json = json;
    if (processed_json.contains("agent") && processed_json["agent"].is_object()) {
        processed_json["agent"] = json::array { processed_json["agent"] };
    }

    std::string error_key;
    if (!InterfaceData().check_json(processed_json, error_key)) {
        LogError << "json is not an InterfaceData" << VAR(error_key) << VAR(processed_json);
        return std::nullopt;
    }

    auto data = processed_json.as<InterfaceData>();

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

std::optional<InterfaceData::ImportData> Parser::parse_import_data(const json::value& json)
{
    std::string error_key;
    if (!InterfaceData::ImportData().check_json(json, error_key)) {
        LogError << "json is not a valid ImportData" << VAR(error_key);
        return std::nullopt;
    }

    return json.as<InterfaceData::ImportData>();
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
