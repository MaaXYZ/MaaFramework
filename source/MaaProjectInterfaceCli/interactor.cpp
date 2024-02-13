#include "interactor.h"

#include <unordered_set>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

// return [1, size]
int input(size_t size, std::string_view prompt = "Please input")
{
    std::cout << std::format("{} [1-{}]: ", prompt, size);
    int value = 0;

    while (true) {
        std::cin >> value;
        if (value > 0 && value <= size) {
            break;
        }
        std::cout << std::format("Invalid value, {} [1-{}]: ", prompt, size);
    }

    return value;
}

void clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Interactor::interact_for_first_time_use()
{
    select_resource();
    add_task();
}

bool Interactor::load(const std::filesystem::path& project_dir)
{
    LogFunc << VAR(project_dir);

    if (!config_.load(project_dir)) {
        return false;
    }

    if (!config_.check_configuration()) {
        std::cout << "\n### The interface has changed and incompatible configurations have been deleted. ###\n\n";
    }

    return true;
}

void Interactor::interact()
{
    if (config_.is_first_time_use()) {
        interact_for_first_time_use();
        config_.save();
    }

    while (true) {
        interact_once();
        config_.save();
    }
}

void Interactor::interact_once()
{
    clear_screen();
    std::cout << "\n### Current configuration ###\n\n";
    std::cout << "Resource:\n\n";
    std::cout << "\t" << MaaNS::utf8_to_crt(config_.configuration().resource) << "\n\n";
    std::cout << "Tasks:\n\n";
    print_config_tasks(false);

    std::cout << "\n\n### Select action ###\n\n";
    std::cout << "\t1. Add task\n";
    std::cout << "\t2. Delete task\n";
    std::cout << "\t3. Move task\n";
    std::cout << "\t4. Switch resource\n";
    std::cout << "\t5. Run tasks\n";
    std::cout << "\n";

    int action = input(6);

    switch (action) {
    case 1:
        add_task();
        break;
    case 2:
        delete_task();
        break;
    case 3:
        move_task();
        break;
    case 4:
        select_resource();
        break;
    case 5:
        // run_tasks();
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
            std::cout << MaaNS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_resources[i].name));
        }
        std::cout << "\n";
        index = input(all_resources.size()) - 1;
    }
    else {
        std::cout << "\n\n\n";
        std::cout << "### Only one resource, use it ###\n\n";
        index = 0;
    }
    const auto& resource = all_resources[index];

    config_.configuration().resource = resource.name;
}

void Interactor::add_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_data_tasks = config_.interface_data().task;
    if (all_data_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "\n\n\n";
    std::cout << "### Add task ###\n\n";
    for (size_t i = 0; i < all_data_tasks.size(); ++i) {
        std::cout << MaaNS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_data_tasks[i].name));
    }
    std::cout << "\n";
    int input_index = input(all_data_tasks.size()) - 1;
    const auto& data_task = all_data_tasks[input_index];

    std::vector<Configuration::Option> config_options;
    for (const auto& option_name : data_task.option) {
        if (!config_.interface_data().option.contains(option_name)) {
            LogError << "Option not found" << VAR(option_name);
            return;
        }

        const auto& opt = config_.interface_data().option.at(option_name);

        if (!opt.default_case.empty()) {
            config_options.emplace_back(Configuration::Option { option_name, opt.default_case });
            continue;
        }
        std::cout << MaaNS::utf8_to_crt(std::format("\n\n\n## Input option for \"{}\" ##\n\n", option_name));
        for (size_t i = 0; i < opt.cases.size(); ++i) {
            std::cout << MaaNS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, opt.cases[i].name));
        }
        std::cout << "\n";

        input_index = input(opt.cases.size()) - 1;
        config_options.emplace_back(Configuration::Option { option_name, opt.cases[input_index].name });
    }

    config_.configuration().task.emplace_back(
        Configuration::Task { .name = data_task.name, .option = std::move(config_options) });
}

void Interactor::edit_task()
{
    // TODO
}

void Interactor::delete_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto& all_config_tasks = config_.configuration().task;
    if (all_config_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "\n\n\n### Delete task ###\n\n";

    print_config_tasks();

    int input_index = input(all_config_tasks.size()) - 1;

    all_config_tasks.erase(all_config_tasks.begin() + input_index);
}

void Interactor::move_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto& all_config_tasks = config_.configuration().task;
    if (all_config_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "\n\n\n### Move task ###\n\n";

    print_config_tasks(true);

    int from_index = input(all_config_tasks.size(), "From") - 1;
    int to_index = input(all_config_tasks.size(), "To") - 1;

    auto task = std::move(all_config_tasks[from_index]);
    all_config_tasks.erase(all_config_tasks.begin() + from_index);
    all_config_tasks.insert(all_config_tasks.begin() + to_index, std::move(task));
}

void Interactor::print_config_tasks(bool with_index) const
{
    auto& all_config_tasks = config_.configuration().task;

    for (size_t i = 0; i < all_config_tasks.size(); ++i) {
        const auto& task = all_config_tasks[i];
        if (with_index) {
            std::cout << MaaNS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, task.name));
        }
        else {
            std::cout << MaaNS::utf8_to_crt(std::format("\t- {}\n", task.name));
        }

        for (const auto& [key, value] : task.option) {
            std::cout << "\t\t- " << MaaNS::utf8_to_crt(key) << ": " << MaaNS::utf8_to_crt(value) << "\n";
        }
    }
    std::cout << "\n";
}