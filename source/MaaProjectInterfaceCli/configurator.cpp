#include "configurator.h"

#include <ranges>

#include "ProjectInterface/Parser.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

using namespace MAA_PROJECT_INTERFACE_NS;

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
    std::ofstream ofs(project_dir_ / kConfigFilename);
    ofs << config_.to_json();
}

std::optional<RuntimeParam> Configurator::generate_runtime() const
{
    RuntimeParam runtime;

    auto resource_iter =
        std::ranges::find_if(data_.resource, [&](const auto& resource) { return resource.name == config_.resource; });

    if (resource_iter == data_.resource.end()) {
        LogWarn << "Resource not found";
        return std::nullopt;
    }

    runtime.resource_path = resource_iter->path;

    for (const auto& config_task : config_.task) {
        auto task_opt = generate_runtime_task(config_task);
        if (!task_opt) {
            LogWarn << "Task not found, ignore" << VAR(config_task.name);
            continue;
        }
        runtime.task.emplace_back(*std::move(task_opt));
    }

    runtime.recognizer = data_.recognizer;
    runtime.action = data_.action;

    LogTrace << VAR(runtime);
    return runtime;
}

std::optional<RuntimeParam::Task> Configurator::generate_runtime_task(const Configuration::Task& config_task) const
{
    // TODO

    std::ignore = config_task;
    return std::nullopt;

    // auto data_iter = std::ranges::find_if(data_.entry, [&](const auto& task) { return task.name == config_task.name;
    // }); if (data_iter == data_.entry.end()) {
    //     LogWarn << "Task not found, remove" << VAR(config_task.name);
    //     return std::nullopt;
    // }
    // const Entry& data_entry = *data_iter;

    // RuntimeParam::Task result { .entry = data_entry.name, .param = data_entry.param };

    // for (const auto& [config_option, config_option_value] : config_task.option) {
    //     auto data_option_iter =
    //         std::ranges::find_if(data_entry.option, [&](const auto& opt) { return opt.name == config_option; });
    //     if (data_option_iter == data_entry.option.end()) {
    //         LogWarn << "Option not found, remove" << VAR(config_task.name) << VAR(config_option);
    //         return std::nullopt;
    //     }
    //     const Option& data_option = *data_option_iter;

    //    auto case_iter =
    //        std::ranges::find_if(data_option.cases, [&](const auto& c) { return c.name == config_option_value; });
    //    if (case_iter == data_option.cases.end()) {
    //        LogWarn << "Case not found, remove" << VAR(config_task.name) << VAR(config_option)
    //                << VAR(config_option_value);
    //        return std::nullopt;
    //    }
    //    const Option::Case& data_case = *case_iter;

    //    // data_case first, duplicate keys will be overwritten by data_case.param
    //    result.param = data_case.param | std::move(result.param);
    //}

    // return result;
}
