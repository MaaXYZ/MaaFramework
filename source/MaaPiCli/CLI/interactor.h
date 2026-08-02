#pragma once

#include <optional>

#include "ProjectInterface/Configurator.h"
#include "ProjectInterface/Types.h"

#include "task_config.h"

class Interactor
{
public:
    explicit Interactor(std::filesystem::path user_path);

    bool load(const std::filesystem::path& resource_path, bool interactive = true);
    void apply_task_config(const TaskConfig& task_cfg);
    void list_task() const;
    bool generate_pi_config(
        const std::string& controller_name,
        const std::string& adb_filter,
        const std::string& resource_name,
        bool force,
        bool default_only = false);
    bool generate_task_config(bool force, const std::string& output_path = {}, bool default_only = false) const;
    void print_config() const;
    void interact();
    bool run(int progress_level = 1);

private:
    void interact_for_first_time_use();

    void welcome() const;
    bool interact_once();

    void select_controller();
    void select_adb();
    void select_adb_auto_detect();
    void select_adb_manual_input();

    bool select_win32_hwnd(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::Win32Config& win32_config);
    void select_macos(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::MacOSConfig& macos_config);
    void select_playcover(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::PlayCoverConfig& playcover_config);
    void select_gamepad(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::GamepadConfig& gamepad_config);
    void select_wlroots();
    void select_wlroots_auto_detect();
    void select_wlroots_manual_input();

    void select_resource();
    void add_task();
    void add_default_tasks();
    void edit_task();
    void delete_task();
    void move_task();
    void apply_preset();

    void process_level_options(
        const std::vector<std::string>& option_names,
        std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option>& config_options,
        const std::string& level_label);

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
    static std::string format_gamepad_config(const MAA_PROJECT_INTERFACE_NS::Configuration::GamepadConfig& gamepad_config);

    // 获取翻译后的显示名称：优先使用翻译后的 label，否则使用 name
    std::string get_display_name(const std::string& name, const std::string& label) const;

    // 读取文本内容：如果是文件路径则读取文件，否则直接返回；支持翻译
    std::string read_text_content(const std::string& text) const;

    // 查找当前配置中选中的 Controller 定义
    const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller* find_current_controller() const;

    // generate-config pi 内部辅助：
    // 根据 controller_name / adb_filter 推断并设置 cfg.controller
    // 返回 false 表示指定的 controller 不存在
    bool resolve_controller(const std::string& controller_name, const std::string& adb_filter);

    // generate-config pi 内部辅助：
    // 根据 resource_name 设置 cfg.resource；为空时取第一个
    // 返回 false 表示指定的 resource 不存在
    bool resolve_resource(const std::string& resource_name);

    // generate-config pi 内部辅助：
    // 探测 ADB 设备，取第一个 name/path/address 含 filter 的写入 cfg.adb
    // filter 为空时取第一个设备；找不到匹配时返回 false
    bool find_and_set_adb_device(const std::string& filter);

    // generate-config pi 内部辅助：
    // 用匹配当前 resource/controller 的任务非交互式填充 cfg.task
    // default_only=true 时只填充 default_check=true 的任务
    void fill_default_tasks(bool default_only = false);

    // 返回满足过滤条件的任务指针列表（指向 interface_data().task 元素）
    // default_only=true：仅保留 default_check==true 的任务
    // filter_env=true：按当前 cfg.resource / cfg.controller.name 过滤
    std::vector<const MAA_PROJECT_INTERFACE_NS::InterfaceData::Task*> filter_tasks(bool default_only, bool filter_env) const;

    // 解析单个 option 的默认值：
    // 优先使用 default_case；无默认值时 Select/Switch 取第一个 case，Input 取各字段 default_，
    // Checkbox 无默认时留空（不强制勾选第一项）
    // 找不到 option 或无有效默认值时返回 nullopt
    std::optional<MAA_PROJECT_INTERFACE_NS::Configuration::Option> resolve_option_default(const std::string& option_name) const;

    enum class ElevationResult
    {
        NotNeeded,       // 不需要提权
        Failed,          // 提权失败
        ElevatedStarted, // 已启动提权进程,当前进程应退出
    };

    // 在运行前检查是否需要管理员权限，如需提权则保存配置并重启
    ElevationResult check_and_elevate_if_needed();

private:
    MAA_PROJECT_INTERFACE_NS::Configurator config_;
    std::filesystem::path user_path_;
};
