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

    void check_config();
    void select_resource();

    const auto& get_runtime() const { return runtime_; }

private:
    std::filesystem::path project_dir_;

    MAA_PROJECT_INTERFACE_NS::InterfaceData data_;
    MAA_PROJECT_INTERFACE_NS::Configuration config_;
    MAA_PROJECT_INTERFACE_NS::RuntimeParam runtime_;
};
