#include "ProjectInterface/Configurator.h"

#include <ranges>

#include "ProjectInterface/Parser.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Configurator::load(const std::filesystem::path& project_dir)
{
    LogFunc << VAR(project_dir);

    auto data_opt = Parser::parse_interface(project_dir / kInterfaceFilename);
    if (!data_opt) {
        LogError << "Failed to parse interface.json";
        return false;
    }
    data_ = *std::move(data_opt);
    if (data_.resource.empty()) {
        LogError << "Resource is empty";
        return false;
    }

    if (auto cfg_opt = Parser::parse_config(project_dir / kConfigFilename)) {
        config_ = *std::move(cfg_opt);
        first_time_use_ = false;
    }
    else {
        first_time_use_ = true;
    }

    project_dir_ = project_dir;
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

void Configurator::save()
{
    std::filesystem::create_directories((project_dir_ / kConfigFilename).parent_path());

    std::ofstream ofs(project_dir_ / kConfigFilename);
    ofs << config_.to_json();
}

std::optional<RuntimeParam> Configurator::generate_runtime() const
{
    constexpr std::string_view kProjectDir = "{PROJECT_DIR}";

    RuntimeParam runtime;

    auto resource_iter = std::ranges::find_if(data_.resource, [&](const auto& resource) {
        return resource.name == config_.resource;
    });

    if (resource_iter == data_.resource.end()) {
        LogWarn << "Resource not found";
        return std::nullopt;
    }

    for (const auto& path_string : resource_iter->path) {
        auto dst = MaaNS::string_replace_all(
            path_string,
            kProjectDir,
            MaaNS::path_to_utf8_string(project_dir_));
        runtime.resource_path.emplace_back(dst);
    }

    for (const auto& config_task : config_.task) {
        auto task_opt = generate_runtime_task(config_task);
        if (!task_opt) {
            LogWarn << "failed to generate runtime, ignore" << VAR(config_task.name);
            continue;
        }
        runtime.task.emplace_back(*std::move(task_opt));
    }

    runtime.recognizer = data_.recognizer;
    runtime.action = data_.action;
    for (auto& [name, executor] : runtime.recognizer) {
        for (auto& param : executor.exec_param) {
            MaaNS::string_replace_all_(
                param,
                kProjectDir,
                MaaNS::path_to_utf8_string(project_dir_));
        }
    }
    for (auto& [name, executor] : runtime.action) {
        for (auto& param : executor.exec_param) {
            MaaNS::string_replace_all_(
                param,
                kProjectDir,
                MaaNS::path_to_utf8_string(project_dir_));
        }
    }

    auto controller_iter = std::ranges::find_if(data_.controller, [&](const auto& controller) {
        return controller.name == config_.controller.name;
    });
    if (controller_iter == data_.controller.end()) {
        LogWarn << "Controller not found" << VAR(config_.controller.name);
        return std::nullopt;
    }
    auto& controller = *controller_iter;

    if (controller.type == InterfaceData::Controller::kTypeAdb) {
        RuntimeParam::AdbParam adb;

        adb.adb_path = config_.adb.adb_path;
        adb.address = config_.adb.address;
        adb.controller_type = controller.adb.touch | controller.adb.key | controller.adb.screencap;
        adb.config = controller.adb.config.dumps();
        adb.agent_path = MaaNS::path_to_utf8_string(project_dir_ / "MaaAgentBinary");

        runtime.controller_param = std::move(adb);
    }
    else if (controller.type == InterfaceData::Controller::kTypeWin32) {
        RuntimeParam::Win32Param win32;

        win32.hwnd = config_.win32.hwnd;
        win32.controller_type =
            controller.win32.touch | controller.win32.key | controller.win32.screencap;

        runtime.controller_param = std::move(win32);
    }
    else {
        LogError << "Unknown controller type" << controller.type;
        return std::nullopt;
    }

    return runtime;
}

std::optional<RuntimeParam::Task>
    Configurator::generate_runtime_task(const Configuration::Task& config_task) const
{
    auto data_iter = std::ranges::find_if(data_.task, [&](const auto& data_task) {
        return data_task.name == config_task.name;
    });
    if (data_iter == data_.task.end()) {
        LogWarn << "task not found" << VAR(config_task.name);
        return std::nullopt;
    }
    const auto& data_task = *data_iter;

    RuntimeParam::Task runtime_task { .name = data_task.name,
                                      .entry = data_task.entry,
                                      .param = data_task.param };

    for (const auto& [config_option, config_option_value] : config_task.option) {
        auto data_option_iter =
            std::ranges::find_if(data_.option, [&](const auto& data_option_pair) {
                return data_option_pair.first == config_option;
            });
        if (data_option_iter == data_.option.end()) {
            LogWarn << "option not found" << VAR(config_option);
            continue;
        }
        const auto& data_option = data_option_iter->second;

        auto data_case_iter = std::ranges::find_if(data_option.cases, [&](const auto& data_case) {
            return data_case.name == config_option_value;
        });
        if (data_case_iter == data_option.cases.end()) {
            LogWarn << "case not found" << VAR(config_option_value);
            continue;
        }
        const auto& data_case = *data_case_iter;

        // data_case first, duplicate keys will be overwritten by data_case.param
        runtime_task.param = data_case.param | std::move(runtime_task.param);
    }

    return runtime_task;
}

MAA_PROJECT_INTERFACE_NS_END
