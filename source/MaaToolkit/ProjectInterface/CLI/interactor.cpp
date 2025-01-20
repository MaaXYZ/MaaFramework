#include "interactor.h"

#include <algorithm>
#include <format>
#include <functional>
#include <ranges>
#include <regex>
#include <unordered_set>

#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/AdbDevice/MaaToolkitAdbDevice.h"
#include "MaaToolkit/DesktopWindow/MaaToolkitDesktopWindow.h"
#include "ProjectInterface/Runner.h"
#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

static bool s_eof = false;

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

        if (std::cin.eof()) {
            s_eof = true;
            return {};
        }

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
        if (s_eof) {
            return {};
        }
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

Interactor::Interactor(
    std::filesystem::path user_path,
    MaaNotificationCallback notify,
    void* notify_trans_arg,
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomRecognitionSession> custom_recognitions,
    std::map<std::string, MAA_PROJECT_INTERFACE_NS::CustomActionSession> custom_actions)
    : user_path_(std::move(user_path))
    , notify_(notify)
    , notify_trans_arg_(notify_trans_arg)
    , custom_recognitions_(std::move(custom_recognitions))
    , custom_actions_(std::move(custom_actions))
{
    LogDebug << VAR(user_path_);
}

bool Interactor::load(const std::filesystem::path& resource_path)
{
    LogFunc << VAR(resource_path);

    if (!config_.load(resource_path, user_path_)) {
        mpause();
        return false;
    }

    if (!config_.check_configuration()) {
        std::cout << "### The interface has changed and incompatible configurations have been "
                     "deleted. ###\n\n";
        mpause();
    }

    return true;
}

void Interactor::interact()
{
    if (config_.is_first_time_use()) {
        interact_for_first_time_use();
        config_.save(user_path_);
    }

    while (true) {
        print_config();
        if (!interact_once()) {
            break;
        }
        config_.save(user_path_);
    }
}

bool Interactor::run()
{
    if (!check_validity()) {
        LogError << "Config is invalid";
        return false;
    }

    auto runtime = config_.generate_runtime();
    if (!runtime) {
        LogError << "Failed to generate runtime";
        return false;
    }

    bool ret = MAA_PROJECT_INTERFACE_NS::Runner::run(runtime.value(), on_maafw_notify, this, custom_recognitions_, custom_actions_);

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
    using namespace MAA_PROJECT_INTERFACE_NS;

    clear_screen();

    welcome();
    std::cout << "### Current configuration ###\n\n";

    std::cout << "Controller:\n\n";
    std::cout << "\t" << MAA_NS::utf8_to_crt(config_.configuration().controller.name) << "\n";

    switch (config_.configuration().controller.type_enum) {
    case InterfaceData::Controller::Type::Adb:
        std::cout << MAA_NS::utf8_to_crt(
            std::format("\t\t{}\n\t\t{}\n", config_.configuration().adb.adb_path, config_.configuration().adb.address));
        break;
    case InterfaceData::Controller::Type::Win32:
        if (config_.configuration().win32.hwnd) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n", format_win32_config(config_.configuration().win32)));
        }
        break;
    default:
        LogError << "Unknown controller type";
        break;
    }

    std::cout << "\n";

    std::cout << "Resource:\n\n";
    std::cout << "\t" << MAA_NS::utf8_to_crt(config_.configuration().resource) << "\n\n";

    std::cout << "Tasks:\n\n";
    print_config_tasks(false);
}

void Interactor::interact_for_first_time_use()
{
    welcome();
    select_controller();
    select_resource();
    add_task();
}

void Interactor::welcome() const
{
    if (config_.interface_data().message.empty()) {
        std::cout << "Welcome to use Maa Project Interface CLI!\n";
    }
    else {
        std::cout << MAA_NS::utf8_to_crt(config_.interface_data().message) << "\n";
    }
    std::cout << "MaaFramework: " << MAA_VERSION << "\n\n";

    std::cout << "Version: " << MAA_NS::utf8_to_crt(config_.interface_data().version) << "\n\n";
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
    if (s_eof) {
        return false;
    }

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
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_controllers = config_.interface_data().controller;
    if (all_controllers.empty()) {
        LogError << "Controller is empty";
        return;
    }

    int index = 0;
    if (all_controllers.size() != 1) {
        std::cout << "### Select controller ###\n\n";
        for (size_t i = 0; i < all_controllers.size(); ++i) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_controllers[i].name));
        }
        std::cout << "\n";
        index = input(all_controllers.size()) - 1;
    }
    else {
        index = 0;
    }
    const auto& controller = all_controllers[index];

    config_.configuration().controller.name = controller.name;

    switch (controller.type_enum) {
    case InterfaceData::Controller::Type::Adb:
        config_.configuration().controller.type_enum = InterfaceData::Controller::Type::Adb;
        select_adb();
        break;
    case InterfaceData::Controller::Type::Win32:
        config_.configuration().controller.type_enum = InterfaceData::Controller::Type::Win32;
        select_win32_hwnd(controller.win32);
        break;
    default:
        LogError << "Unknown controller type" << VAR(controller.type);
        break;
    }
}

void Interactor::select_adb()
{
    std::cout << "### Select ADB ###\n\n";

    std::cout << "\t1. Auto detect\n";
    std::cout << "\t2. Manual input\n";
    std::cout << "\n";

    int action = input(2);

    switch (action) {
    case 1:
        select_adb_auto_detect();
        break;

    case 2:
        select_adb_manual_input();
        break;
    }
}

void Interactor::select_adb_auto_detect()
{
    std::cout << "Finding device...\n\n";

    auto list_handle = MaaToolkitAdbDeviceListCreate();
    OnScopeLeave([&]() { MaaToolkitAdbDeviceListDestroy(list_handle); });

    MaaToolkitAdbDeviceFind(list_handle);

    size_t size = MaaToolkitAdbDeviceListSize(list_handle);
    if (size == 0) {
        std::cout << "No device found!\n\n";
        select_adb();
        return;
    }

    std::cout << "## Select Device ##\n\n";

    for (size_t i = 0; i < size; ++i) {
        auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, i);

        std::string name = MaaToolkitAdbDeviceGetName(device_handle);
        std::string path = MaaToolkitAdbDeviceGetAdbPath(device_handle);
        std::string address = MaaToolkitAdbDeviceGetAddress(device_handle);

        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n\t\t{}\n\t\t{}\n", i + 1, name, path, address));
    }
    std::cout << "\n";

    int index = input(size) - 1;
    auto& adb_config = config_.configuration().adb;

    auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, index);

    adb_config.adb_path = MaaToolkitAdbDeviceGetAdbPath(device_handle);
    adb_config.address = MaaToolkitAdbDeviceGetAddress(device_handle);
    adb_config.config = json::parse(MaaToolkitAdbDeviceGetConfig(device_handle)).value_or(json::object()).as_object();
}

void Interactor::select_adb_manual_input()
{
    std::cout << "Please input ADB path: ";
    std::cin.sync();
    std::string adb_path;
    std::getline(std::cin, adb_path);
    config_.configuration().adb.adb_path = adb_path;
    std::cout << "\n";

    std::cout << "Please input ADB address: ";
    std::cin.sync();
    std::string adb_address;
    std::getline(std::cin, adb_address);
    config_.configuration().adb.address = adb_address;
    std::cout << "\n";
}

std::string Interactor::format_win32_config(const MAA_PROJECT_INTERFACE_NS::Configuration::Win32Config& win32_config)
{
    return std::format(
        "{}\n\t\t{}\n\t\t{}",
        win32_config.hwnd,
        MAA_NS::from_u16(win32_config.class_name),
        MAA_NS::from_u16(win32_config.window_name));
}

bool Interactor::select_win32_hwnd(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::Win32Config& win32_config)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto list_handle = MaaToolkitDesktopWindowListCreate();
    OnScopeLeave([&]() { MaaToolkitDesktopWindowListDestroy(list_handle); });

    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t list_size = MaaToolkitDesktopWindowListSize(list_handle);

    auto class_u16 = MAA_NS::to_u16(win32_config.class_regex);
    auto window_u16 = MAA_NS::to_u16(win32_config.window_regex);
    if (!MAA_NS::regex_valid(class_u16) || !MAA_NS::regex_valid(window_u16)) {
        LogError << "regex is invalid" << VAR(class_u16) << VAR(window_u16);
        return false;
    }
    std::wregex class_regex(class_u16);
    std::wregex window_regex(window_u16);

    std::vector<Configuration::Win32Config> matched_config;
    for (size_t i = 0; i < list_size; ++i) {
        Configuration::Win32Config rt_config;

        auto window_handle = MaaToolkitDesktopWindowListAt(list_handle, i);
        rt_config.hwnd = MaaToolkitDesktopWindowGetHandle(window_handle);
        rt_config.class_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetClassName(window_handle));
        rt_config.window_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetWindowName(window_handle));

        std::wsmatch class_match;
        std::wsmatch window_match;
        if (std::regex_search(rt_config.class_name, class_match, class_regex)
            && std::regex_search(rt_config.window_name, window_match, window_regex)) {
            matched_config.emplace_back(std::move(rt_config));
        }
    }

    if (matched_config.empty()) {
        LogError << "Window Not Found" << VAR(win32_config.class_regex) << VAR(win32_config.window_regex);
        mpause();
        return false;
    }
    size_t matched_size = matched_config.size();
    if (matched_size == 1) {
        config_.configuration().win32 = matched_config.front();
        return true;
    }

    std::cout << "### Select HWND ###\n\n";

    for (size_t i = 0; i < matched_size; ++i) {
        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, format_win32_config(matched_config.at(i))));
    }
    std::cout << "\n";

    int index = input(matched_size) - 1;
    config_.configuration().win32 = matched_config.at(index);

    return true;
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
        std::cout << "### Select resource ###\n\n";
        for (size_t i = 0; i < all_resources.size(); ++i) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_resources[i].name));
        }
        std::cout << "\n";
        index = input(all_resources.size()) - 1;
    }
    else {
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

    std::cout << "### Add task ###\n\n";
    for (size_t i = 0; i < all_data_tasks.size(); ++i) {
        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, all_data_tasks[i].name));
    }
    std::cout << "\n";
    auto input_indexes = input_multi(all_data_tasks.size());

    for (int index : input_indexes) {
        const auto& data_task = all_data_tasks[index - 1];

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
            std::cout << MAA_NS::utf8_to_crt(std::format("\n\n## Input option of \"{}\" for \"{}\" ##\n\n", option_name, data_task.name));
            for (size_t i = 0; i < opt.cases.size(); ++i) {
                std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, opt.cases[i].name));
            }
            std::cout << "\n";

            int case_index = input(opt.cases.size()) - 1;
            config_options.emplace_back(Configuration::Option { option_name, opt.cases[case_index].name });
        }

        config_.configuration().task.emplace_back(Configuration::Task { .name = data_task.name, .option = std::move(config_options) });
    }
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

    std::cout << "### Delete task ###\n\n";

    print_config_tasks();

    auto input_indexes = input_multi(all_config_tasks.size());

    std::unordered_set<int> indexes(input_indexes.begin(), input_indexes.end());
    std::vector<int> sorted_indexes(indexes.begin(), indexes.end());
    std::sort(sorted_indexes.begin(), sorted_indexes.end(), std::greater<int>());

    for (int index : sorted_indexes) {
        all_config_tasks.erase(all_config_tasks.begin() + index - 1);
    }
}

void Interactor::move_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto& all_config_tasks = config_.configuration().task;
    if (all_config_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    std::cout << "### Move task ###\n\n";

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
            std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, task.name));
        }
        else {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t- {}\n", task.name));
        }

        for (const auto& [key, value] : task.option) {
            std::cout << "\t\t- " << MAA_NS::utf8_to_crt(key) << ": " << MAA_NS::utf8_to_crt(value) << "\n";
        }
    }
    std::cout << "\n";
}

bool Interactor::check_validity()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    if (config_.configuration().controller.type_enum == InterfaceData::Controller::Type::Win32
        && config_.configuration().win32.hwnd == nullptr) {
        auto& name = config_.configuration().controller.name;
        auto controller_iter = std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

        if (controller_iter == config_.interface_data().controller.end()) {
            LogError << "Contorller not found" << VAR(name);
            return false;
        }

        return select_win32_hwnd(controller_iter->win32);
    }

    return true;
}

void Interactor::mpause() const
{
    std::cout << "\nPress Enter to continue...";
    std::cin.sync();
    std::cin.get();
}

void Interactor::on_maafw_notify(const char* msg, const char* details_json, void* notify_trans_arg)
{
    if (std::string(msg).starts_with("Tasker.Task")) {
        std::string entry = json::parse(details_json).value_or(json::value())["entry"].as_string();
        std::cout << MAA_NS::utf8_to_crt(std::format("on_maafw_notify: {} {}", msg, entry)) << std::endl;
    }

    Interactor* pthis = static_cast<Interactor*>(notify_trans_arg);
    if (pthis && pthis->notify_) {
        pthis->notify_(msg, details_json, pthis->notify_trans_arg_);
    }
}
