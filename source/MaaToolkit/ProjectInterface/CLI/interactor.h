#pragma once

#include "MaaFramework/MaaDef.h"
#include "ProjectInterface/Configurator.h"

class Interactor
{
public:
    bool load(
        const std::filesystem::path& project_dir,
        MaaNotificationCallback callback,
        void* callback_arg,
        std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognizerSession> custom_recognizers,
        std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions);
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

    void select_resource();
    void add_task();
    void edit_task();
    void delete_task();
    void move_task();

    void print_config_tasks(bool with_index = true) const;

    bool check_validity();

    void mpause() const;

    static void on_maafw_notify(const char* msg, const char* details_json, void* callback_arg);
    static std::string format_win32_config(const MAA_PROJECT_INTERFACE_NS::Configuration::Win32Config& win32_config);

private:
    MAA_PROJECT_INTERFACE_NS::Configurator config_;
    MaaNotificationCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognizerSession> custom_recognizers_;
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions_;
};
