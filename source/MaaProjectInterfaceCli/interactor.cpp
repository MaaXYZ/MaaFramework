#include "interactor.h"

#include <unordered_set>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

// return [1, size]
int input(size_t size)
{
    std::cout << std::format("Please Input [1-{}]:", size);
    int value = 0;

    while (true) {
        std::cin >> value;
        if (value > 0 && value <= size) {
            break;
        }
        std::cout << std::format("Invalid value, please Input [1-{}]:", size);
    }

    return value;
}

void Interactor::interact_for_first_time_use()
{
    select_resource();
    add_task();
    interact();
}

void Interactor::interact()
{
    while (true) {
        interact_once();
    }
}

void Interactor::interact_once()
{
    std::cout << "\n\n\n";
    std::cout << "### Current configuration ###\n\n";
    std::cout << "Resource:\n\n";
    std::cout << "\t" << config_.configuration().resource << "\n\n";
    std::cout << "Tasks:\n\n";
    for (const auto& task : config_.configuration().task) {
        std::cout << "\t- " << task.name << "\n";
        for (const auto& [key, value] : task.option) {
            std::cout << "\t\t- " << key << ": " << value << "\n";
        }
    }

    std::cout << "### Select action ###\n\n";
    std::cout << "\t1. Run tasks\n";
    std::cout << "\t2. Select resource\n";
    std::cout << "\t3. Add task\n";
    std::cout << "\t4. Edit task\n";
    std::cout << "\t5. Remove task\n";
    std::cout << "\t6. Move task\n";

    int action = input(6);

    switch (action) {
    case 1:
        // run_tasks();
        break;
    case 2:
        select_resource();
        break;
    case 3:
        add_task();
        break;
    case 4:
        edit_task();
        break;
    case 5:
        remove_task();
        break;
    case 6:
        move_task();
        break;
    }
}

void Interactor::select_resource()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_resources = config_.interface_data().resource;
    if (all_resources.empty()) {
        LogError << "Resource is empty";
        return;
    }

    int index = 0;
    if (all_resources.size() != 1) {
        std::cout << "\n\n\n";
        std::cout << "### Select resource ###\n\n";
        for (size_t i = 0; i < all_resources.size(); ++i) {
            std::cout << std::format("\t{}. {}\n", i + 1, all_resources[i].name);
        }
        index = input(all_resources.size()) - 1;
    }
    else {
        std::cout << "\n\n\n";
        std::cout << "### Only one resource, use it ###\n\n";
        index = 0;
    }
    const Resource& resource = all_resources[index];

    config_.configuration().resource = resource.name;
    config_.save();
}

void Interactor::add_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_tasks = config_.interface_data().entry;
    if (all_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "\n\n\n";
    std::cout << "### Add task ###\n\n";
    for (size_t i = 0; i < all_tasks.size(); ++i) {
        std::cout << std::format("\t{}. {}\n", i + 1, all_tasks[i].name);
    }
    int task_index = input(all_tasks.size()) - 1;
    const Entry& task = all_tasks[task_index];

    std::unordered_map<std::string, std::string> option_map;
    for (const Option& opt : task.options) {
        if (!opt.default_case.empty()) {
            option_map.emplace(opt.name, opt.default_case);
            continue;
        }
        std::cout << std::format("\n\n\n## Input option for \"{}\" ##\n\n", opt.name);
        for (size_t i = 0; i < opt.cases.size(); ++i) {
            std::cout << std::format("\t{}. {}\n", i + 1, opt.cases[i]);
        }
        int case_index = input(opt.cases.size()) - 1;
        option_map.emplace(opt.name, opt.cases[case_index]);
    }

    config_.configuration().task.emplace_back(
        Configuration::Task { .name = task.name, .option = std::move(option_map) });
    config_.save();
}

void Interactor::edit_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_tasks = config_.configuration().task;
    if (all_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "\n\n\n";
    std::cout << "### Edit task ###\n\n";
    for (size_t i = 0; i < all_tasks.size(); ++i) {
        std::cout << std::format("\t{}. {}\n", i + 1, all_tasks[i].name);
    }
    int task_index = input(all_tasks.size()) - 1;

    Configuration::Task& task = config_.configuration().task[task_index];

    const auto& all_tasks_in_interface = config_.interface_data().entry;
    auto task_in_interface =
        std::ranges::find_if(all_tasks_in_interface, [&](const auto& entry) { return entry.name == task.name; });

    if (task_in_interface == all_tasks_in_interface.end()) {
        LogError << "Task not found in interface" << VAR(task.name);
        return;
    }

    // TODO
}

void Interactor::remove_task() {}

void Interactor::move_task() {}
