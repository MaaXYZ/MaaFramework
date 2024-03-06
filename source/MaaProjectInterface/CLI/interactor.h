#pragma once

#include "MaaFramework/MaaDef.h"
#include "ProjectInterface/Configurator.h"

class Interactor
{
public:
    bool load(const std::filesystem::path& project_dir);
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

    void select_resource();
    void add_task();
    void edit_task();
    void delete_task();
    void move_task();

    void print_config_tasks(bool with_index = true) const;

    void mpause() const;

    static void on_maafw_notify(
        MaaStringView msg,
        MaaStringView details_json,
        MaaTransparentArg callback_arg);

private:
    MAA_PROJECT_INTERFACE_NS::Configurator config_;
};
