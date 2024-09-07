#include "interactor.h"

#include <algorithm>
#include <format>
#include <functional>
#include <ranges>
#include <regex>
#include <sstream>
#include <unordered_set>

#include "Buffer/BufferTypes.hpp"
#include "Buffer/StringBuffer.hpp"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaPI/MaaPIAPI.h"
#include "MaaToolkit/AdbDevice/MaaToolkitAdbDevice.h"
#include "MaaToolkit/DesktopWindow/MaaToolkitDesktopWindow.h"
#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/ScopeLeave.hpp"

// return [1, size]
std::vector<int> input_multi_impl(size_t size, std::string_view prompt)
{
    std::vector<int> values;

    auto fail = [&]() {
        std::cout << std::format("Invalid value, {} [1-{}]: ", prompt, size);
        values.clear();
    };

    while (true) {
        std::cin.sync();
        std::string buffer;
        std::getline(std::cin, buffer);

        if (buffer.empty()) {
            fail();
            continue;
        }

        if (!std::ranges::all_of(buffer, [](char c) { return std::isdigit(c) || std::isspace(c); })) {
            fail();
            continue;
        }

        std::istringstream iss(buffer);
        size_t val = 0;
        while (iss >> val) {
            if (val == 0 || val > size) {
                fail();
                break;
            }
            values.emplace_back(static_cast<int>(val));
        }
        break;
    }

    return values;
}

// return [1, size]
int input(size_t size, std::string_view prompt = "Please input")
{
    std::cout << std::format("{} [1-{}]: ", prompt, size);

    auto fail = [&]() {
        std::cout << std::format("Invalid value, {} [1-{}]: ", prompt, size);
    };

    int val = 0;
    while (true) {
        auto values = input_multi_impl(size, prompt);
        if (values.size() != 1) {
            fail();
            continue;
        }
        val = values.front();
        break;
    }
    std::cout << "\n";

    return val;
}

std::vector<int> input_multi(size_t size, std::string_view prompt = "Please input multiple")
{
    std::cout << std::format("{} [1-{}]: ", prompt, size);
    auto values = input_multi_impl(size, prompt);
    std::cout << "\n";
    return values;
}

void clear_screen()
{
#ifdef _WIN32
    std::ignore = system("cls");
#else
    std::ignore = system("clear");
#endif
}

std::optional<std::string> read_file(const std::filesystem::path& filepath)
{
    std::ifstream ifs(filepath, std::ios::in);
    if (!ifs.is_open()) {
        return std::nullopt;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    ifs.close();
    return oss.str();
}

Interactor::Interactor()
{
    data_ = MaaPIDataCreate();
    config_ = MaaPIConfigCreate();
    runtime_ = MaaPIRuntimeCreate();
    MaaPIRuntimeBind(runtime_, data_, config_);
    client_ = MaaPIClientCreate("zh-Hans", &Interactor::on_maafw_client_handler, this);
}

bool Interactor::load(const std::filesystem::path& project_dir)
{
    LogFunc << VAR(project_dir);

    project_dir_ = project_dir;

    auto data_json = read_file(project_dir / "pi.json");
    if (!data_json) {
        LogError << "Failed to read pi.json";
        mpause();
        return false;
    }

    if (!MaaPIDataLoad(data_, data_json->c_str(), MAA_NS::path_to_utf8_string(project_dir).c_str())) {
        LogError << "Failed to load pi.json";
        mpause();
        return false;
    }

    auto gen_def = true;
    auto config_json = read_file(project_dir / "pi_config.json");
    if (config_json) {
        if (MaaPIConfigLoad(config_, data_, config_json->c_str())) {
            gen_def = false;
        }
        else {
            LogWarn << "Failed to load pi_config.json";
        }
    }
    if (gen_def) {
        if (!MaaPIConfigGenDef(config_, data_)) {
            LogError << "Failed to generate default pi_config.json";
            return false;
        }
    }

    save_config();

    return true;
}

void Interactor::interact()
{
    while (true) {
        print_config();
        if (!interact_once()) {
            break;
        }
        save_config();
    }
}

bool Interactor::run()
{
    if (!MaaPIRuntimeSetup(runtime_, &Interactor::on_maafw_notify, this)) {
        LogError << "Failed to generate runtime";
        return false;
    }

    bool ret = MaaPIClientPerform(client_, runtime_, MaaPIClientAction_LaunchTask);

    if (!ret) {
        std::cout << "### Failed to run tasks ###\n\n";
    }
    else {
        std::cout << "### All tasks have been completed ###\n\n";
    }

    return ret;
}

void Interactor::print_config() const
{
    clear_screen();

    welcome();
    std::cout << "### Current configuration ###\n\n";

    // std::cout << "Controller:\n\n";
    // std::cout << "\t" << MAA_NS::utf8_to_crt(config_.configuration().controller.name) << "\n";

    // switch (config_.configuration().controller.type_enum) {
    // case InterfaceData::Controller::Type::Adb:
    //     std::cout << MAA_NS::utf8_to_crt(std::format(
    //         "\t\t{}\n\t\t{}\n",
    //         MaaNS::path_to_utf8_string(config_.configuration().adb.adb_path),
    //         config_.configuration().adb.address));
    //     break;
    // case InterfaceData::Controller::Type::Win32:
    //     if (config_.configuration().win32.hwnd) {
    //         std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n", format_win32_config(config_.configuration().win32)));
    //     }
    //     break;
    // default:
    //     LogError << "Unknown controller type";
    //     break;
    // }

    // std::cout << "\n";

    // std::cout << "Resource:\n\n";
    // std::cout << "\t" << MAA_NS::utf8_to_crt(config_.configuration().resource) << "\n\n";

    // std::cout << "Tasks:\n\n";
    // print_config_tasks(false);
}

void Interactor::welcome() const
{
    // if (config_.interface_data().message.empty()) {
    //     std::cout << "Welcome to use Maa Project Interface CLI!\n";
    // }
    // else {
    //     std::cout << MAA_NS::utf8_to_crt(config_.interface_data().message) << "\n";
    // }
    std::cout << "MaaFramework: " << MAA_VERSION << "\n\n";

    // std::cout << "Version: " << MAA_NS::utf8_to_crt(config_.interface_data().version) << "\n\n";
}

bool Interactor::interact_once()
{
    std::cout << "### Select action ###\n\n";
    std::cout << "\t1. Switch controller\n";
    std::cout << "\t2. Switch resource\n";
    std::cout << "\t3. Add task\n";
    std::cout << "\t4. Move task\n";
    std::cout << "\t5. Delete task\n";
    std::cout << "\t6. Run tasks\n";
    std::cout << "\t7. Exit\n";
    std::cout << "\n";

    int action = input(7);

    switch (action) {
    case 1:
        select_controller();
        break;
    case 2:
        select_resource();
        break;
    case 3:
        add_task();
        break;
    case 4:
        move_task();
        break;
    case 5:
        delete_task();
        break;
    case 6:
        run();
        mpause();
        break;
    case 7:
        return false;
    }

    return true;
}

void Interactor::select_controller()
{
    std::cout << "### Select controller ###\n\n";

    client_handler_ = [&](const auto& info) -> int32_t {
        for (size_t i = 0; i < info.choices.size(); i++) {
            std::cout << MAA_NS::utf8_to_crt(std::format("{}: {}", i + 1, info.choices[i].choice)) << std::endl;
        }
        return input(info.choices.size()) - 1;
    };

    MaaPIClientPerform(client_, runtime_, MaaPIClientAction_SelectController);
}

void Interactor::select_resource()
{
    std::cout << "### Select resource ###\n\n";

    client_handler_ = [&](const auto& info) -> int32_t {
        for (size_t i = 0; i < info.choices.size(); i++) {
            std::cout << MAA_NS::utf8_to_crt(std::format("{}: {}", i + 1, info.choices[i].choice)) << std::endl;
        }
        return input(info.choices.size()) - 1;
    };

    MaaPIClientPerform(client_, runtime_, MaaPIClientAction_SelectResource);
}

void Interactor::add_task()
{
    // using namespace MAA_PROJECT_INTERFACE_NS;

    // const auto& all_data_tasks = config_.interface_data().task;
    // if (all_data_tasks.empty()) {
    //     LogError << "Task is empty";
    //     return;
    // }

    // std::cout << "### Add task ###\n\n";
    // for (size_t i = 0; i < all_data_tasks.size(); ++i) {
    //     std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_data_tasks[i].name));
    // }
    // std::cout << "\n";
    // auto input_indexes = input_multi(all_data_tasks.size());

    // for (int index : input_indexes) {
    //     const auto& data_task = all_data_tasks[index - 1];

    //     std::vector<Configuration::Option> config_options;
    //     for (const auto& option_name : data_task.option) {
    //         if (!config_.interface_data().option.contains(option_name)) {
    //             LogError << "Option not found" << VAR(option_name);
    //             return;
    //         }

    //         const auto& opt = config_.interface_data().option.at(option_name);

    //         if (!opt.default_case.empty()) {
    //             config_options.emplace_back(Configuration::Option { option_name, opt.default_case });
    //             continue;
    //         }
    //         std::cout << MAA_NS::utf8_to_crt(std::format("\n\n## Input option of \"{}\" for \"{}\" ##\n\n", option_name,
    //         data_task.name)); for (size_t i = 0; i < opt.cases.size(); ++i) {
    //             std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, opt.cases[i].name));
    //         }
    //         std::cout << "\n";

    //         int case_index = input(opt.cases.size()) - 1;
    //         config_options.emplace_back(Configuration::Option { option_name, opt.cases[case_index].name });
    //     }

    //     config_.configuration().task.emplace_back(Configuration::Task { .name = data_task.name, .option = std::move(config_options) });
    // }
}

void Interactor::edit_task()
{
    // TODO
}

void Interactor::delete_task()
{
    // using namespace MAA_PROJECT_INTERFACE_NS;

    // auto& all_config_tasks = config_.configuration().task;
    // if (all_config_tasks.empty()) {
    //     LogError << "Task is empty";
    //     return;
    // }

    // std::cout << "### Delete task ###\n\n";

    // print_config_tasks();

    // auto input_indexes = input_multi(all_config_tasks.size());

    // std::unordered_set<int> indexes(input_indexes.begin(), input_indexes.end());
    // std::vector<int> sorted_indexes(indexes.begin(), indexes.end());
    // std::sort(sorted_indexes.begin(), sorted_indexes.end(), std::greater<int>());

    // for (int index : sorted_indexes) {
    //     all_config_tasks.erase(all_config_tasks.begin() + index - 1);
    // }
}

void Interactor::move_task()
{
    // using namespace MAA_PROJECT_INTERFACE_NS;

    // auto& all_config_tasks = config_.configuration().task;
    // if (all_config_tasks.empty()) {
    //     LogError << "Task is empty";
    //     return;
    // }

    // std::cout << "### Move task ###\n\n";

    // print_config_tasks(true);

    // int from_index = input(all_config_tasks.size(), "From") - 1;
    // int to_index = input(all_config_tasks.size(), "To") - 1;

    // auto task = std::move(all_config_tasks[from_index]);
    // all_config_tasks.erase(all_config_tasks.begin() + from_index);
    // all_config_tasks.insert(all_config_tasks.begin() + to_index, std::move(task));
}

void Interactor::print_config_tasks(bool with_index) const
{
    std::ignore = with_index;
    // auto& all_config_tasks = config_.configuration().task;

    // for (size_t i = 0; i < all_config_tasks.size(); ++i) {
    //     const auto& task = all_config_tasks[i];
    //     if (with_index) {
    //         std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, task.name));
    //     }
    //     else {
    //         std::cout << MAA_NS::utf8_to_crt(std::format("\t- {}\n", task.name));
    //     }

    //     for (const auto& [key, value] : task.option) {
    //         std::cout << "\t\t- " << MAA_NS::utf8_to_crt(key) << ": " << MAA_NS::utf8_to_crt(value) << "\n";
    //     }
    // }
    // std::cout << "\n";
}

void Interactor::mpause() const
{
    std::cout << "\nPress Enter to continue...";
    std::cin.sync();
    std::cin.get();
}

bool Interactor::save_config() const
{
    MaaStringBuffer* str = MaaStringBufferCreate();
    OnScopeLeave([str]() { MaaStringBufferDestroy(str); });
    if (!MaaPIConfigSave(config_, str)) {
        LogError << "Failed to serialize pi_config.json";
        return false;
    }
    std::ofstream ofs(project_dir_ / "pi_config.json");
    ofs << str->get();
    return true;
}

void Interactor::on_maafw_notify(const char* msg, const char* details_json, void* callback_arg)
{
    Interactor* pthis = static_cast<Interactor*>(callback_arg);
    std::ignore = pthis;

    std::string entry = json::parse(details_json).value_or(json::value())["entry"].as_string();
    std::cout << MAA_NS::utf8_to_crt(std::format("on_maafw_notify: {} {}", msg, entry)) << std::endl;
}

int32_t Interactor::on_maafw_client_handler(
    MaaPIClient* client,
    MaaPIClientAction action,
    MaaStringListBuffer* choice,
    MaaStringListBuffer* details,
    void* arg)
{
    std::ignore = client;
    std::ignore = action;

    auto self = reinterpret_cast<Interactor*>(arg);

    if (!self->client_handler_) {
        return -1;
    }

    ClientHandleInfo info;

    auto count = choice->size();
    for (size_t i = 0; i < count; i++) {
        info.choices.push_back({
            choice->at(i).get(),
            json::parse(details->at(i).get())->as_object(),
        });
    }

    return self->client_handler_(info);
}
