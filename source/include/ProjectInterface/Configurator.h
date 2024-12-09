#pragma once

#include <filesystem>
#include <string_view>

#include "Types.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Configurator
{
    static constexpr std::string_view kInterfaceFilename = "interface.json";
    static constexpr std::string_view kConfigPath = "config/maa_pi_config.json";

public:
    bool load(const std::filesystem::path& project_dir, const std::filesystem::path& user_dir);
    bool check_configuration();
    void save(const std::filesystem::path& user_dir);

    std::optional<RuntimeParam> generate_runtime() const;

    const auto& interface_data() const { return data_; }

    const auto& configuration() const { return config_; }

    auto& configuration() { return config_; }

    bool is_first_time_use() const { return first_time_use_; }

private:
    std::optional<RuntimeParam::Task> generate_runtime_task(const Configuration::Task& config_task) const;

    std::filesystem::path resource_dir_;

    InterfaceData data_;
    bool first_time_use_ = false;
    Configuration config_;
};

MAA_PROJECT_INTERFACE_NS_END
