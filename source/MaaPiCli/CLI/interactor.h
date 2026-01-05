#pragma once

#include "ProjectInterface/Configurator.h"

class Interactor
{
public:
    explicit Interactor(std::filesystem::path user_path);

    bool load(const std::filesystem::path& resource_path);
    void print_config() const;
    void interact();
    bool run();

private:
    void interact_for_first_time_use();

    void welcome() const;
    bool interact_once();

    void select_controller();
    void select_adb();
    void select_adb_auto_detect();
    void select_adb_manual_input();

    bool select_win32_hwnd(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::Win32Config& win32_config);
    void select_playcover(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::PlayCoverConfig& playcover_config);

    void select_resource();
    void add_task();
    void add_default_tasks();
    void edit_task();
    void delete_task();
    void move_task();

    // Process option and its nested sub-options recursively
    // Returns false if option processing failed (e.g., option not found, invalid configuration)
    bool process_option(
        const std::string& option_name,
        const std::string& task_display_name,
        std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option>& config_options);

    void print_config_tasks(bool with_index = true) const;

    bool check_validity();

    void mpause() const;

    static std::string format_win32_config(const MAA_PROJECT_INTERFACE_NS::Configuration::Win32Config& win32_config);

    // 获取翻译后的显示名称：优先使用翻译后的 label，否则使用 name
    std::string get_display_name(const std::string& name, const std::string& label) const;

    // 读取文本内容：如果是文件路径则读取文件，否则直接返回；支持翻译
    std::string read_text_content(const std::string& text) const;

private:
    MAA_PROJECT_INTERFACE_NS::Configurator config_;
    std::filesystem::path user_path_;
};
