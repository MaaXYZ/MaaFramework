#pragma once

#include "configurator.h"

class Interactor
{
public:
    const auto& configurator() const { return config_; }
    auto& configurator() { return config_; }

    void interact_for_first_time_use();
    void interact();

private:
    void interact_once();
    void select_resource();
    void add_task();
    void edit_task();
    void remove_task();
    void move_task();

    Configurator config_;
};
