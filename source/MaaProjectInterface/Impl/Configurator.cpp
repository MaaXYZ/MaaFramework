#include "ProjectInterface/Configurator.h"

#include <ranges>

#include "ProjectInterface/Parser.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Configurator::load(const std::filesystem::path& resource_dir, const std::filesystem::path& user_dir)
{
    LogFunc << VAR(resource_dir) << VAR(user_dir);

    auto data_opt = Parser::parse_interface(resource_dir / kInterfaceFilename);
    if (!data_opt) {
        LogError << "Failed to parse interface.json";
        return false;
    }
    data_ = *std::move(data_opt);
    if (data_.resource.empty()) {
        LogError << "Resource is empty";
        return false;
    }

    if (auto cfg_opt = Parser::parse_config(user_dir / kConfigPath)) {
        config_ = *std::move(cfg_opt);
        first_time_use_ = false;
    }
    else {
        first_time_use_ = true;
    }

    resource_dir_ = resource_dir;
    return true;
}

bool Configurator::check_configuration()
{
    LogFunc;

    if (first_time_use_) {
        return true;
    }

    return Parser::check_configuration(data_, config_);
}

void Configurator::save(const std::filesystem::path& user_dir)
{
    LogInfo << VAR(user_dir);

    const auto config_path = user_dir / kConfigPath;
    if (config_path.has_parent_path()) {
        std::filesystem::create_directories(config_path.parent_path());
    }

    std::ofstream ofs(config_path);
    if (!ofs.is_open()) {
        LogError << "failed to open" << VAR(config_path);
        return;
    }

    ofs << config_.to_json();
}

std::optional<RuntimeParam> Configurator::generate_runtime() const
{
    constexpr std::string_view kProjectDir = "{PROJECT_DIR}";

    RuntimeParam runtime;

    auto resource_iter = std::ranges::find_if(data_.resource, [&](const auto& resource) { return resource.name == config_.resource; });

    if (resource_iter == data_.resource.end()) {
        LogWarn << "Resource not found";
        return std::nullopt;
    }

    std::string resource_dir = MaaNS::path_to_utf8_string(resource_dir_);
    for (const auto& path_string : resource_iter->path) {
        auto dst = MaaNS::string_replace_all(path_string, kProjectDir, resource_dir);
        runtime.resource_path.emplace_back(MaaNS::path(dst));
    }
    if (runtime.resource_path.empty()) {
        LogWarn << "No resource to load";
        return std::nullopt;
    }

    for (const auto& config_task : config_.task) {
        auto task_opt = generate_runtime_task(config_task);
        if (!task_opt) {
            LogWarn << "failed to generate runtime, ignore" << VAR(config_task.name);
            continue;
        }
        runtime.task.emplace_back(*std::move(task_opt));
    }
    if (runtime.task.empty()) {
        LogWarn << "No task to run";
        return std::nullopt;
    }

    auto controller_iter =
        std::ranges::find_if(data_.controller, [&](const auto& controller) { return controller.name == config_.controller.name; });
    if (controller_iter == data_.controller.end()) {
        LogWarn << "Controller not found" << VAR(config_.controller.name);
        return std::nullopt;
    }
    auto& controller = *controller_iter;

    switch (controller.type_enum) {
    case InterfaceData::Controller::Type::Adb: {
        RuntimeParam::AdbParam adb;

        adb.adb_path = config_.adb.adb_path;
        adb.address = config_.adb.address;
        adb.screencap = controller.adb.screencap;
        adb.input = controller.adb.input;
        adb.config = (controller.adb.config | config_.adb.config).dumps();
        adb.agent_path = MaaNS::path_to_utf8_string(resource_dir_ / "MaaAgentBinary");

        runtime.controller_param = std::move(adb);
    } break;

    case InterfaceData::Controller::Type::Win32: {
        RuntimeParam::Win32Param win32;

        win32.hwnd = config_.win32.hwnd;
        win32.screencap = controller.win32.screencap;
        win32.input = controller.win32.input;

        runtime.controller_param = std::move(win32);
    } break;

    default: {
        LogError << "Unknown controller type" << controller.type;
        return std::nullopt;
    }
    }

    runtime.gpu = config_.gpu;

    if (!data_.agent.child_exec.empty()) {
        RuntimeParam::Agent agent;

        agent.child_exec = MaaNS::path(MaaNS::string_replace_all(data_.agent.child_exec, kProjectDir, resource_dir));
        agent.identifier = data_.agent.identifier;

        for (const auto& arg : data_.agent.child_args) {
            auto dst = MaaNS::string_replace_all(arg, kProjectDir, resource_dir);
            agent.child_args.emplace_back(std::move(dst));
        }

        runtime.agent = std::move(agent);
    }

    return runtime;
}

std::optional<RuntimeParam::Task> Configurator::generate_runtime_task(const Configuration::Task& config_task) const
{
    auto data_iter = std::ranges::find_if(data_.task, [&](const auto& data_task) { return data_task.name == config_task.name; });
    if (data_iter == data_.task.end()) {
        LogWarn << "task not found" << VAR(config_task.name);
        return std::nullopt;
    }
    const auto& data_task = *data_iter;

    RuntimeParam::Task runtime_task { .name = data_task.name, .entry = data_task.entry, .pipeline_override = data_task.pipeline_override };

    for (const auto& [config_option, config_option_value] : config_task.option) {
        auto data_option_iter =
            std::ranges::find_if(data_.option, [&](const auto& data_option_pair) { return data_option_pair.first == config_option; });
        if (data_option_iter == data_.option.end()) {
            LogWarn << "option not found" << VAR(config_option);
            continue;
        }
        const auto& data_option = data_option_iter->second;

        auto data_case_iter =
            std::ranges::find_if(data_option.cases, [&](const auto& data_case) { return data_case.name == config_option_value; });
        if (data_case_iter == data_option.cases.end()) {
            LogWarn << "case not found" << VAR(config_option_value);
            continue;
        }
        const auto& data_case = *data_case_iter;

        // data_case first, duplicate keys will be overwritten by data_case.param
        runtime_task.pipeline_override = data_case.pipeline_override | std::move(runtime_task.pipeline_override);
    }

    return runtime_task;
}

MAA_PROJECT_INTERFACE_NS_END
