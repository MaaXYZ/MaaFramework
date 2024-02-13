#pragma once

#include "configurator.h"

class Interactor
{
public:
    bool load(const std::filesystem::path& project_dir);
    void interact();
    std::optional<MAA_PROJECT_INTERFACE_NS::RuntimeParam> generate_runtime() const;

private:
    void interact_for_first_time_use();

    bool interact_once();
    void select_resource();
    void add_task();
    void edit_task();
    void delete_task();
    void move_task();

    void print_config_tasks(bool with_index = true) const;

private:
    Configurator config_;
};
