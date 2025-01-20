#pragma once

#include "MaaFramework/MaaDef.h"
#include "ProjectInterface/Configurator.h"

class Interactor
{
public:
    Interactor(
        std::filesystem::path user_path,
        MaaNotificationCallback notify,
        void* notify_trans_arg,
        std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognitionSession> custom_recognitions,
        std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions);

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

    void select_resource();
    void add_task();
    void edit_task();
    void delete_task();
    void move_task();

    void print_config_tasks(bool with_index = true) const;

    bool check_validity();

    void mpause() const;

    static void on_maafw_notify(const char* msg, const char* details_json, void* notify_trans_arg);
    static std::string format_win32_config(const MAA_PROJECT_INTERFACE_NS::Configuration::Win32Config& win32_config);

private:
    MAA_PROJECT_INTERFACE_NS::Configurator config_;
    std::filesystem::path user_path_;
    MaaNotificationCallback notify_ = nullptr;
    void* notify_trans_arg_ = nullptr;
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognitionSession> custom_recognitions_;
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions_;
};
