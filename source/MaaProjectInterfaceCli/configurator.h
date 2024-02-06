#pragma once

#include <filesystem>
#include <string_view>

#include "ProjectInterface/Types.h"

class Configurator
{
    static constexpr std::string_view kInterfaceFilename = "interface.json";
    static constexpr std::string_view kConfigFilename = "config/config.json";

public:
    bool load(const std::filesystem::path& project_dir);
    void save();

    std::optional<MAA_PROJECT_INTERFACE_NS::RuntimeParam> generate_runtime() const;

    const auto& interface_data() const { return data_; }

    const auto& configuration() const { return config_; }
    auto& configuration() { return config_; }

private:
    std::optional<MAA_PROJECT_INTERFACE_NS::RuntimeParam::Task> generate_runtime_task(
        const MAA_PROJECT_INTERFACE_NS::Configuration::Task& config_task) const;

    std::filesystem::path project_dir_;

    MAA_PROJECT_INTERFACE_NS::InterfaceData data_;
    MAA_PROJECT_INTERFACE_NS::Configuration config_;
};
