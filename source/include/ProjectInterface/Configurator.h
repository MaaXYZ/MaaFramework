#pragma once

#include <filesystem>
#include <string_view>

#include "Types.h"

#include "Common/Conf.h"

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

    const auto& resource_dir() const { return resource_dir_; }

    // 国际化文本翻译：如果文本以 $ 开头，则从翻译表中查找
    std::string translate(const std::string& text) const;

private:
    std::optional<RuntimeParam::Task> generate_runtime_task(const Configuration::Task& config_task) const;
    void load_translations();
    std::string detect_system_language() const;

    std::filesystem::path resource_dir_;

    InterfaceData data_;
    bool first_time_use_ = false;
    Configuration config_;
    std::unordered_map<std::string, std::string> translations_; // 翻译表
};

MAA_PROJECT_INTERFACE_NS_END
