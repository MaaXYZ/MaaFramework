#include "Impl/PIConfig.h"

#include <set>

#include "Impl/PIData.h"
#include "PI/Types.h"

MAA_PI_NS_BEGIN

bool PIConfig::load(MaaPIData* _data, std::string_view json)
{
    PIData* data = dynamic_cast<PIData*>(_data);
    if (!data) {
        return false;
    }

    auto json_value = json::parse(json);
    if (!json_value) {
        return false;
    }

    ConfigJson config;
    if (!config.from_json(json_value.value())) {
        return false;
    }

    data_ = nullptr;
    controller_ = nullptr;
    controller_info_ = std::monostate {};
    resource_ = nullptr;
    task_.clear();

    data_ = data;

    {
        auto ctrl = data_->controller_.find(config.controller);
        if (ctrl == data_->controller_.end()) {
            return false;
        }
        controller_ = &ctrl->second;
    }

    {
        if (auto adb = std::get_if<PIData::ControllerInfo::AdbInfo>(&controller_->info)) {
            std::ignore = adb;
            controller_info_ = config.adb;
        }
        else if (auto desktop = std::get_if<PIData::ControllerInfo::DesktopInfo>(&controller_->info)) {
            std::ignore = desktop;
            controller_info_ = DesktopConfigInfo {};
        }
        else {
            return false;
        }
    }

    {
        auto res = data_->resource_.find(config.resource);
        if (res == data_->resource_.end()) {
            return false;
        }
        resource_ = &res->second;
    }

    {
        for (const auto& task_cfg : config.task) {
            auto task = data_->task_.find(task_cfg.name);
            if (task == data_->task_.end()) {
                return false;
            }
            TaskConfigInfo info = { &task->second };
            std::set<std::string> expect_opts = { task->second.option.begin(), task->second.option.end() };
            for (const auto& opt_cfg : task_cfg.option) {
                auto opt = data_->option_.find(opt_cfg.name);
                if (opt == data_->option_.end()) {
                    return false;
                }
                expect_opts.erase(opt_cfg.name);
                auto cs = opt->second.cases.find(opt_cfg.value.empty() ? opt->second.default_case : opt_cfg.value);
                if (cs == opt->second.cases.end()) {
                    return false;
                }
                info.option_.push_back({ &opt->second, &cs->second });
            }
            for (const auto& omit_name : expect_opts) {
                auto opt = data_->option_.find(omit_name);
                if (opt == data_->option_.end()) {
                    return false;
                }
                auto cs = opt->second.cases.find(opt->second.default_case);
                if (cs == opt->second.cases.end()) {
                    return false;
                }
                info.option_.push_back({ &opt->second, &cs->second });
            }
            task_.push_back(std::move(info));
        }
    }

    return true;
}

bool PIConfig::gen_default(MaaPIData* _data)
{
    PIData* data = dynamic_cast<PIData*>(_data);
    if (!data) {
        return false;
    }

    data_ = nullptr;
    controller_ = nullptr;
    controller_info_ = std::monostate {};
    resource_ = nullptr;
    task_.clear();

    data_ = data;

    {
        auto ctrl = data_->controller_.find(data_->default_controller_);
        if (ctrl == data_->controller_.end()) {
            return false;
        }
        controller_ = &ctrl->second;
    }

    {
        if (auto adb = std::get_if<PIData::ControllerInfo::AdbInfo>(&controller_->info)) {
            std::ignore = adb;
            controller_info_ = AdbConfigInfo {};
        }
        else if (auto desktop = std::get_if<PIData::ControllerInfo::DesktopInfo>(&controller_->info)) {
            std::ignore = desktop;
            controller_info_ = DesktopConfigInfo {};
        }
        else {
            return false;
        }
    }

    {
        auto res = data_->resource_.find(data_->default_resource_);
        if (res == data_->resource_.end()) {
            return false;
        }
        resource_ = &res->second;
    }

    {
        ConfigJson::Task task_cfg = { data_->default_task_, {} };
        auto task = data_->task_.find(data_->default_task_);
        if (task == data_->task_.end()) {
            return false;
        }
        TaskConfigInfo info = { &task->second };
        for (const auto& opt_name : task->second.option) {
            auto opt = data_->option_.find(opt_name);
            if (opt == data_->option_.end()) {
                return false;
            }
            auto cs = opt->second.cases.find(opt->second.default_case);
            if (cs == opt->second.cases.end()) {
                return false;
            }
            info.option_.push_back({ &opt->second, &cs->second });
        }
        task_.push_back(std::move(info));
    }

    return true;
}

bool PIConfig::save(std::string& json)
{
    if (!data_) {
        return false;
    }

    ConfigJson result;

    result.controller = controller_->name;
    if (auto adb = std::get_if<AdbConfigInfo>(&controller_info_)) {
        result.adb = *adb;
    }
    else if (auto desktop = std::get_if<DesktopConfigInfo>(&controller_info_)) {
        std::ignore = desktop;
        result.desktop = { 0 };
    }
    result.resource = resource_->name;
    for (const auto& task : task_) {
        std::vector<ConfigJson::Task::Option> opts;
        for (const auto& opt : task.option_) {
            opts.push_back({ opt.option_->name, opt.case_->name });
        }
        result.task.push_back({ task.task_->name, std::move(opts) });
    }

    json = result.to_json().format();

    return true;
}

MAA_PI_NS_END
