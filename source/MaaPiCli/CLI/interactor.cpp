#include "interactor.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <ranges>
#include <unordered_set>

#include <boost/regex.hpp>

#if defined(_WIN32)
#include "MaaUtils/SafeWindows.hpp"
#include <shellapi.h>
#endif

#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/AdbDevice/MaaToolkitAdbDevice.h"
#include "MaaToolkit/DesktopWindow/MaaToolkitDesktopWindow.h"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ProjectInterface/Runner.h"

static bool s_eof = false;

#if defined(__APPLE__)
static constexpr bool kPlayCoverSupported = true;
#else
static constexpr bool kPlayCoverSupported = false;
#endif

#if defined(_WIN32)
static constexpr bool kGamepadSupported = true;
#else
static constexpr bool kGamepadSupported = false;
#endif

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

        if (!std::ranges::all_of(buffer, [](unsigned char c) { return std::isdigit(c) || std::isspace(c); })) {
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

namespace
{
#if defined(_WIN32)
// Windows path limit: MAX_PATH (260) is insufficient for long paths; use a larger buffer.
// Modern Windows supports paths up to 32,767 characters with proper configuration.
constexpr DWORD kMaxPathBuffer = 32768;

// ShellExecuteW return value threshold: values > 32 indicate success, <= 32 indicate error codes.
// See: https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew
constexpr intptr_t kShellExecuteSuccessThreshold = 32;

bool is_running_as_admin()
{
    BOOL is_member = FALSE;

    // CheckTokenMembership(nullptr, ...) checks the current effective token (UAC-aware).
    BYTE sid_buffer[SECURITY_MAX_SID_SIZE] = {};
    DWORD sid_size = sizeof(sid_buffer);
    PSID admin_sid = sid_buffer;

    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, admin_sid, &sid_size)) {
        return false;
    }
    if (!CheckTokenMembership(nullptr, admin_sid, &is_member)) {
        return false;
    }
    return is_member == TRUE;
}

std::optional<std::wstring> get_current_exe_path()
{
    std::wstring buf;
    buf.resize(kMaxPathBuffer);
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (len == 0 || len >= buf.size()) {
        return std::nullopt;
    }
    buf.resize(len);
    return buf;
}

bool restart_self_as_admin()
{
    auto exe = get_current_exe_path();
    if (!exe) {
        return false;
    }

    // Preserve original command-line arguments (excluding argv[0], the executable path)
    std::wstring combined_params;
    int argc = 0;
    LPWSTR cmd_line = GetCommandLineW();
    LPWSTR* argv = nullptr;

    if (cmd_line != nullptr) {
        argv = CommandLineToArgvW(cmd_line, &argc);
    }

    if (argv != nullptr && argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (!combined_params.empty()) {
                combined_params.push_back(L' ');
            }
            combined_params.append(argv[i]);
        }
        LocalFree(argv);
    }

    const wchar_t* lpParameters = combined_params.empty() ? nullptr : combined_params.c_str();

    const auto ret = reinterpret_cast<intptr_t>(ShellExecuteW(nullptr, L"runas", exe->c_str(), lpParameters, nullptr, SW_SHOWNORMAL));
    return ret > kShellExecuteSuccessThreshold;
}
#endif
} // namespace

Interactor::Interactor(std::filesystem::path user_path)
    : user_path_(std::move(user_path))
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

const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller* Interactor::find_current_controller() const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& name = config_.configuration().controller.name;
    auto it = std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

    return (it != config_.interface_data().controller.end()) ? &(*it) : nullptr;
}

Interactor::ElevationResult Interactor::check_and_elevate_if_needed()
{
#if defined(_WIN32)
    const auto* controller = find_current_controller();
    if (!controller || !controller->permission_required || is_running_as_admin()) {
        return ElevationResult::NotNeeded;
    }

    std::cout << "\nThis controller requires administrator privileges.\n"
                 "MaaPiCli will try to restart itself as Administrator to run tasks (UAC prompt will appear).\n\n";

    config_.save(user_path_);

    if (!restart_self_as_admin()) {
        std::cout << "\nFailed to restart as Administrator (UAC may have been cancelled, or the request was denied).\n"
                     "Please manually start MaaPiCli as Administrator and run again.\n\n";
        return ElevationResult::Failed;
    }

    // Elevated instance has been started; caller should exit current process.
    return ElevationResult::ElevatedStarted;
#else
    return ElevationResult::NotNeeded;
#endif
}

bool Interactor::run()
{
    auto elevation_result = check_and_elevate_if_needed();
    if (elevation_result == ElevationResult::Failed) {
        return false;
    }
    if (elevation_result == ElevationResult::ElevatedStarted) {
        // Elevated instance is now running; signal caller to exit gracefully.
        return true;
    }

    if (!check_validity()) {
        LogError << "Config is invalid";
        return false;
    }

    auto runtime = config_.generate_runtime();
    if (!runtime) {
        LogError << "Failed to generate runtime";
        return false;
    }

    bool ret = MAA_PROJECT_INTERFACE_NS::Runner::run(runtime.value());

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

    switch (config_.configuration().controller.type) {
    case InterfaceData::Controller::Type::Adb:
        std::cout << MAA_NS::utf8_to_crt(
            std::format("\t\t{}\n\t\t{}\n", config_.configuration().adb.adb_path, config_.configuration().adb.address));
        break;
    case InterfaceData::Controller::Type::Win32:
        if (config_.configuration().win32.hwnd) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n", format_win32_config(config_.configuration().win32)));
        }
        break;
    case InterfaceData::Controller::Type::PlayCover: {
        const auto& pc = config_.configuration().playcover;
        if (!pc.address.empty() && !pc.uuid.empty()) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n\t\t{}\n", pc.address, pc.uuid));
        }
        if (!kPlayCoverSupported) {
            std::cout << "\t\t(PlayCover is only available on macOS)\n";
        }
    } break;
    case InterfaceData::Controller::Type::Gamepad: {
        if (config_.configuration().gamepad.hwnd) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n", format_gamepad_config(config_.configuration().gamepad)));
        }
        if (!kGamepadSupported) {
            std::cout << "\t\t(Gamepad is only available on Windows)\n";
        }
    } break;
    default:
        LogError << "Unknown controller type" << VAR(config_.configuration().controller.type);
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

    // Auto-add tasks with default_check=true
    add_default_tasks();

    // If no default tasks were added, let user select manually
    if (config_.configuration().task.empty()) {
        add_task();
    }
}

void Interactor::welcome() const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();

    // 显示标题或项目名称
    if (!data.title.empty()) {
        std::cout << MAA_NS::utf8_to_crt(config_.translate(data.title)) << "\n\n";
    }
    else {
        std::string display_name = get_display_name(data.name, data.label);
        if (!display_name.empty()) {
            std::cout << MAA_NS::utf8_to_crt(display_name);
            if (!data.version.empty()) {
                std::cout << " v" << MAA_NS::utf8_to_crt(data.version);
            }
            std::cout << "\n\n";
        }
    }

    // 显示欢迎信息
    if (!data.welcome.empty()) {
        std::string welcome_text = read_text_content(data.welcome);
        std::cout << MAA_NS::utf8_to_crt(welcome_text) << "\n\n";
    }

    // 显示项目描述
    if (!data.description.empty()) {
        std::string desc_text = read_text_content(data.description);
        std::cout << "Description: " << MAA_NS::utf8_to_crt(desc_text) << "\n\n";
    }

    // 显示 GitHub 地址
    if (!data.github.empty()) {
        std::cout << "GitHub: " << MAA_NS::utf8_to_crt(data.github) << "\n\n";
    }

    // 显示联系方式
    if (!data.contact.empty()) {
        std::string contact_text = read_text_content(data.contact);
        std::cout << "Contact: " << MAA_NS::utf8_to_crt(contact_text) << "\n\n";
    }

    // 显示许可证信息
    if (!data.license.empty()) {
        std::string license_text = read_text_content(data.license);
        std::cout << "License: " << MAA_NS::utf8_to_crt(license_text) << "\n\n";
    }
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
    default:
        LogError << "Invalid action" << VAR(action);
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
            const auto& ctrl = all_controllers[i];
            std::string display_name = get_display_name(ctrl.name, ctrl.label);
            std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}", i + 1, display_name));
            if (ctrl.type == InterfaceData::Controller::Type::PlayCover && !kPlayCoverSupported) {
                std::cout << " (macOS only)";
            }
            if (ctrl.type == InterfaceData::Controller::Type::Gamepad && !kGamepadSupported) {
                std::cout << " (Windows only)";
            }
            std::cout << "\n";
            if (!ctrl.description.empty()) {
                std::string desc_text = read_text_content(ctrl.description);
                std::cout << "\t   " << MAA_NS::utf8_to_crt(desc_text) << "\n";
            }
        }
        std::cout << "\n";
        index = input(all_controllers.size()) - 1;
    }
    else {
        index = 0;
    }
    const auto& controller = all_controllers[index];

    config_.configuration().controller.name = controller.name;
    config_.configuration().controller.type = controller.type;

    switch (controller.type) {
    case InterfaceData::Controller::Type::Adb:
        config_.configuration().controller.type = InterfaceData::Controller::Type::Adb;
        select_adb();
        break;
    case InterfaceData::Controller::Type::Win32:
        config_.configuration().controller.type = InterfaceData::Controller::Type::Win32;
        select_win32_hwnd(controller.win32);
        break;
    case InterfaceData::Controller::Type::PlayCover:
        if (!kPlayCoverSupported) {
            std::cout << "\nPlayCover controller is only available on macOS.\n";
            // Check if there are other controllers available
            bool has_other_controllers = std::ranges::any_of(all_controllers, [](const auto& ctrl) {
                return ctrl.type != InterfaceData::Controller::Type::PlayCover;
            });
            if (has_other_controllers) {
                std::cout << "Please select another controller.\n\n";
                mpause();
                select_controller();
            }
            else {
                std::cout << "No other controllers available.\n\n";
                mpause();
            }
            return;
        }
        config_.configuration().controller.type = InterfaceData::Controller::Type::PlayCover;
        select_playcover(controller.playcover);
        break;
    case InterfaceData::Controller::Type::Gamepad:
        if (!kGamepadSupported) {
            std::cout << "\nGamepad controller is only available on Windows.\n";
            // Check if there are other controllers available
            bool has_other_controllers = std::ranges::any_of(all_controllers, [](const auto& ctrl) {
                return ctrl.type != InterfaceData::Controller::Type::Gamepad;
            });
            if (has_other_controllers) {
                std::cout << "Please select another controller.\n\n";
                mpause();
                select_controller();
            }
            else {
                std::cout << "No other controllers available.\n\n";
                mpause();
            }
            return;
        }
        config_.configuration().controller.type = InterfaceData::Controller::Type::Gamepad;
        select_gamepad(controller.gamepad);
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

    adb_config.name = MaaToolkitAdbDeviceGetName(device_handle);
    adb_config.adb_path = MaaToolkitAdbDeviceGetAdbPath(device_handle);
    adb_config.address = MaaToolkitAdbDeviceGetAddress(device_handle);
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

    config_.configuration().adb.name = std::format("{}-{}", adb_address, adb_path);
}

void Interactor::select_playcover(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::PlayCoverConfig& playcover_config)
{
    std::cout << "### Configure PlayCover ###\n\n";

    auto& pc = config_.configuration().playcover;

    // Use uuid from interface.json if available
    if (!playcover_config.uuid.empty() && pc.uuid.empty()) {
        pc.uuid = playcover_config.uuid;
    }

    // Default address if not configured
    std::string default_address = pc.address.empty() ? "127.0.0.1:1717" : pc.address;

    // Ask for address (use default if empty input)
    std::cout << "PlayTools service address (host:port) [" << default_address << "]: ";
    std::cin.sync();
    std::string buffer;
    std::getline(std::cin, buffer);

    if (std::cin.eof()) {
        s_eof = true;
        return;
    }

    pc.address = buffer.empty() ? default_address : buffer;
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

std::string Interactor::format_gamepad_config(const MAA_PROJECT_INTERFACE_NS::Configuration::GamepadConfig& gamepad_config)
{
    std::string type_str = gamepad_config.gamepad_type.empty() ? "Xbox360" : gamepad_config.gamepad_type;
    return std::format(
        "{}\n\t\t{}\n\t\t{}\n\t\tGamepad: {}",
        gamepad_config.hwnd,
        MAA_NS::from_u16(gamepad_config.class_name),
        MAA_NS::from_u16(gamepad_config.window_name),
        type_str);
}

bool Interactor::select_win32_hwnd(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::Win32Config& win32_config)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto list_handle = MaaToolkitDesktopWindowListCreate();
    OnScopeLeave([&]() { MaaToolkitDesktopWindowListDestroy(list_handle); });

    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t list_size = MaaToolkitDesktopWindowListSize(list_handle);

    auto class_regex = MAA_NS::regex_valid(MAA_NS::to_u16(win32_config.class_regex));
    auto window_regex = MAA_NS::regex_valid(MAA_NS::to_u16(win32_config.window_regex));
    if (!class_regex || !window_regex) {
        LogError << "regex is invalid" << VAR(win32_config.class_regex) << VAR(win32_config.window_regex);
        return false;
    }

    std::vector<Configuration::Win32Config> matched_config;
    for (size_t i = 0; i < list_size; ++i) {
        Configuration::Win32Config rt_config;

        auto window_handle = MaaToolkitDesktopWindowListAt(list_handle, i);
        rt_config.hwnd = MaaToolkitDesktopWindowGetHandle(window_handle);
        rt_config.class_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetClassName(window_handle));
        rt_config.window_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetWindowName(window_handle));

        if (boost::regex_search(rt_config.class_name, *class_regex) && boost::regex_search(rt_config.window_name, *window_regex)) {
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

void Interactor::select_gamepad(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::GamepadConfig& gamepad_config)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    std::cout << "### Configure Gamepad Controller ###\n\n";

    // Select window for screencap (optional, reuse Win32 window selection logic)
    auto list_handle = MaaToolkitDesktopWindowListCreate();
    OnScopeLeave([&]() { MaaToolkitDesktopWindowListDestroy(list_handle); });

    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t list_size = MaaToolkitDesktopWindowListSize(list_handle);

    auto class_regex = MAA_NS::regex_valid(MAA_NS::to_u16(gamepad_config.class_regex));
    auto window_regex = MAA_NS::regex_valid(MAA_NS::to_u16(gamepad_config.window_regex));
    if (!class_regex || !window_regex) {
        LogError << "regex is invalid" << VAR(gamepad_config.class_regex) << VAR(gamepad_config.window_regex);
        std::cout << "Window regex is invalid, screencap will not be available.\n";
        config_.configuration().gamepad.hwnd = nullptr;
    }
    else {
        std::vector<Configuration::GamepadConfig> matched_config;
        for (size_t i = 0; i < list_size; ++i) {
            Configuration::GamepadConfig rt_config;

            auto window_handle = MaaToolkitDesktopWindowListAt(list_handle, i);
            rt_config.hwnd = MaaToolkitDesktopWindowGetHandle(window_handle);
            rt_config.class_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetClassName(window_handle));
            rt_config.window_name = MAA_NS::to_u16(MaaToolkitDesktopWindowGetWindowName(window_handle));

            if (boost::regex_search(rt_config.class_name, *class_regex) && boost::regex_search(rt_config.window_name, *window_regex)) {
                matched_config.emplace_back(std::move(rt_config));
            }
        }

        if (matched_config.empty()) {
            std::cout << "No matching window found, screencap will not be available.\n";
            config_.configuration().gamepad.hwnd = nullptr;
        }
        else {
            size_t matched_size = matched_config.size();
            if (matched_size == 1) {
                config_.configuration().gamepad.hwnd = matched_config.front().hwnd;
                config_.configuration().gamepad.class_name = matched_config.front().class_name;
                config_.configuration().gamepad.window_name = matched_config.front().window_name;
            }
            else {
                std::cout << "### Select HWND for screencap ###\n\n";

                for (size_t i = 0; i < matched_size; ++i) {
                    std::cout << MAA_NS::utf8_to_crt(
                        std::format(
                            "\t{}. {}\n\t\t{}\n\t\t{}\n",
                            i + 1,
                            matched_config.at(i).hwnd,
                            MAA_NS::from_u16(matched_config.at(i).class_name),
                            MAA_NS::from_u16(matched_config.at(i).window_name)));
                }
                std::cout << "\n";

                int index = input(matched_size) - 1;
                config_.configuration().gamepad.hwnd = matched_config.at(index).hwnd;
                config_.configuration().gamepad.class_name = matched_config.at(index).class_name;
                config_.configuration().gamepad.window_name = matched_config.at(index).window_name;
            }
        }
    }

    // Select gamepad type
    std::cout << "\n### Select Gamepad Type ###\n\n";
    std::cout << "\t1. Xbox 360\n";
    std::cout << "\t2. DualShock 4 (PS4)\n";
    std::cout << "\n";

    int type_index = input(2);
    config_.configuration().gamepad.gamepad_type = (type_index == 1) ? "Xbox360" : "DualShock4";

    std::cout << "\n";
}

void Interactor::select_resource()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_resources = config_.interface_data().resource;
    const auto& current_controller = config_.configuration().controller.name;

    if (all_resources.empty()) {
        LogError << "Resource is empty";
        return;
    }

    // Filter resources by current controller
    std::vector<const InterfaceData::Resource*> available_resources;
    for (const auto& res : all_resources) {
        // If controller list is empty, resource supports all controllers
        if (res.controller.empty() || std::ranges::find(res.controller, current_controller) != res.controller.end()) {
            available_resources.push_back(&res);
        }
    }

    if (available_resources.empty()) {
        LogError << "No resource available for controller" << VAR(current_controller);
        return;
    }

    int index = 0;
    if (available_resources.size() != 1) {
        std::cout << "### Select resource ###\n\n";
        for (size_t i = 0; i < available_resources.size(); ++i) {
            const auto& res = *available_resources[i];
            std::string display_name = get_display_name(res.name, res.label);
            std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, display_name));
            if (!res.description.empty()) {
                std::string desc_text = read_text_content(res.description);
                std::cout << "\t   " << MAA_NS::utf8_to_crt(desc_text) << "\n";
            }
        }
        std::cout << "\n";
        index = input(available_resources.size()) - 1;
    }
    else {
        index = 0;
    }
    const auto& resource = *available_resources[index];

    config_.configuration().resource = resource.name;
}

void Interactor::add_task()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_data_tasks = config_.interface_data().task;
    const auto& current_resource = config_.configuration().resource;
    const auto& current_controller = config_.configuration().controller.name;

    if (all_data_tasks.empty()) {
        LogError << "Task is empty";
        return;
    }

    // Filter tasks by current resource and controller
    std::vector<const InterfaceData::Task*> available_tasks;
    for (const auto& task : all_data_tasks) {
        // If resource list is empty, task supports all resources
        if (!task.resource.empty() && std::ranges::find(task.resource, current_resource) == task.resource.end()) {
            continue;
        }
        // If controller list is empty, task supports all controllers
        if (!task.controller.empty() && std::ranges::find(task.controller, current_controller) == task.controller.end()) {
            continue;
        }
        available_tasks.push_back(&task);
    }

    if (available_tasks.empty()) {
        LogError << "No task available for resource" << VAR(current_resource) << "and controller" << VAR(current_controller);
        return;
    }

    std::cout << "### Add task ###\n\n";
    for (size_t i = 0; i < available_tasks.size(); ++i) {
        const auto& task = *available_tasks[i];
        std::string display_name = get_display_name(task.name, task.label);
        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, display_name));
        if (!task.description.empty()) {
            std::string desc_text = read_text_content(task.description);
            std::cout << "\t   " << MAA_NS::utf8_to_crt(desc_text) << "\n";
        }
    }
    std::cout << "\n";
    auto input_indexes = input_multi(available_tasks.size());

    for (int index : input_indexes) {
        const auto& data_task = *available_tasks[index - 1];
        std::string task_display_name = get_display_name(data_task.name, data_task.label);

        std::vector<Configuration::Option> config_options;
        bool all_options_ok = true;
        for (const auto& option_name : data_task.option) {
            if (!process_option(option_name, task_display_name, config_options)) {
                LogWarn << "Failed to process option, skipping task" << VAR(data_task.name) << VAR(option_name);
                all_options_ok = false;
                break;
            }
        }

        if (all_options_ok) {
            config_.configuration().task.emplace_back(Configuration::Task { .name = data_task.name, .option = std::move(config_options) });
        }
    }
}

void Interactor::add_default_tasks()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& all_data_tasks = config_.interface_data().task;
    const auto& current_resource = config_.configuration().resource;
    const auto& current_controller = config_.configuration().controller.name;

    for (const auto& task : all_data_tasks) {
        // Skip tasks without default_check
        if (!task.default_check) {
            continue;
        }

        // Check if task supports current resource
        if (!task.resource.empty() && std::ranges::find(task.resource, current_resource) == task.resource.end()) {
            continue;
        }

        // Check if task supports current controller
        if (!task.controller.empty() && std::ranges::find(task.controller, current_controller) == task.controller.end()) {
            continue;
        }

        std::string task_display_name = get_display_name(task.name, task.label);

        // Process options for this task
        std::vector<Configuration::Option> config_options;
        bool all_options_ok = true;
        for (const auto& option_name : task.option) {
            if (!process_option(option_name, task_display_name, config_options)) {
                LogWarn << "Failed to process option for default task, skipping" << VAR(task.name) << VAR(option_name);
                all_options_ok = false;
                break;
            }
        }

        if (all_options_ok) {
            config_.configuration().task.emplace_back(Configuration::Task { .name = task.name, .option = std::move(config_options) });
            std::cout << "Auto-added default task: " << MAA_NS::utf8_to_crt(task_display_name) << "\n";
        }
    }

    if (!config_.configuration().task.empty()) {
        std::cout << "\n";
    }
}

bool Interactor::process_option(
    const std::string& option_name,
    const std::string& task_display_name,
    std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option>& config_options)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    if (!config_.interface_data().option.contains(option_name)) {
        LogError << "Option not found" << VAR(option_name);
        return false;
    }

    const auto& opt = config_.interface_data().option.at(option_name);

    Configuration::Option config_opt;
    config_opt.name = option_name;

    std::string opt_display_name = get_display_name(option_name, opt.label);

    // Selected case for processing sub-options
    const InterfaceData::Option::Case* selected_case = nullptr;

    switch (opt.type) {
    case InterfaceData::Option::Type::Select: {
        if (!opt.default_case.empty()) {
            config_opt.value = opt.default_case;
            // Find the selected case for sub-options
            auto case_iter = std::ranges::find(opt.cases, opt.default_case, std::mem_fn(&InterfaceData::Option::Case::name));
            if (case_iter != opt.cases.end()) {
                selected_case = &(*case_iter);
            }
        }
        else {
            std::cout << MAA_NS::utf8_to_crt(
                std::format("\n\n## Select option \"{}\" for \"{}\" ##\n\n", opt_display_name, task_display_name));
            if (!opt.description.empty()) {
                std::string desc_text = read_text_content(opt.description);
                std::cout << MAA_NS::utf8_to_crt(desc_text) << "\n\n";
            }
            for (size_t i = 0; i < opt.cases.size(); ++i) {
                const auto& case_item = opt.cases[i];
                std::string case_display_name = get_display_name(case_item.name, case_item.label);
                std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, case_display_name));
                if (!case_item.description.empty()) {
                    std::string case_desc = read_text_content(case_item.description);
                    std::cout << "\t   " << MAA_NS::utf8_to_crt(case_desc) << "\n";
                }
            }
            std::cout << "\n";

            int case_index = input(opt.cases.size()) - 1;
            config_opt.value = opt.cases[case_index].name;
            selected_case = &opt.cases[case_index];
        }
    } break;

    case InterfaceData::Option::Type::Switch: {
        // Switch 类型必须有恰好两个 cases
        if (opt.cases.size() < 2) {
            LogError << "Switch option must have at least 2 cases" << VAR(option_name) << VAR(opt.cases.size());
            return false;
        }
        std::cout << MAA_NS::utf8_to_crt(std::format("\n\n## Switch option \"{}\" for \"{}\" ##\n\n", opt_display_name, task_display_name));
        if (!opt.description.empty()) {
            std::string desc_text = read_text_content(opt.description);
            std::cout << MAA_NS::utf8_to_crt(desc_text) << "\n\n";
        }
        std::string case0_name = get_display_name(opt.cases[0].name, opt.cases[0].label);
        std::string case1_name = get_display_name(opt.cases[1].name, opt.cases[1].label);
        std::cout << "\t" << MAA_NS::utf8_to_crt(case0_name) << "\n";
        std::cout << "\t" << MAA_NS::utf8_to_crt(case1_name) << "\n";
        std::cout << "\nInput Y/N: ";

        static const std::unordered_set<std::string> yes_names = { "Yes", "yes", "Y", "y" };
        static const std::unordered_set<std::string> no_names = { "No", "no", "N", "n" };

        std::string buffer;
        bool is_yes = false;
        while (true) {
            std::cin.sync();
            std::getline(std::cin, buffer);

            if (std::cin.eof()) {
                s_eof = true;
                return false;
            }

            if (yes_names.contains(buffer)) {
                is_yes = true;
                break;
            }
            else if (no_names.contains(buffer)) {
                is_yes = false;
                break;
            }
            else {
                std::cout << "Invalid input, please input Y/N: ";
            }
        }

        // Find matching Yes/No case
        auto find_case = [&](bool find_yes) -> const InterfaceData::Option::Case* {
            for (const auto& case_item : opt.cases) {
                bool is_yes_case = yes_names.contains(case_item.name);
                if (find_yes == is_yes_case) {
                    return &case_item;
                }
            }
            // Fallback
            LogWarn << "No matching Yes/No case found, using fallback" << VAR(find_yes);
            return find_yes ? &opt.cases[0] : &opt.cases[1];
        };

        selected_case = find_case(is_yes);
        config_opt.value = selected_case->name;
        std::cout << "\n";
    } break;

    case InterfaceData::Option::Type::Input: {
        std::cout << MAA_NS::utf8_to_crt(std::format("\n\n## Input option \"{}\" for \"{}\" ##\n\n", opt_display_name, task_display_name));
        if (!opt.description.empty()) {
            std::string desc_text = read_text_content(opt.description);
            std::cout << MAA_NS::utf8_to_crt(desc_text) << "\n\n";
        }

        for (const auto& input_def : opt.inputs) {
            std::string default_val = input_def.default_;
            std::string input_display_name = get_display_name(input_def.name, input_def.label);
            if (!input_def.description.empty()) {
                std::string input_desc = read_text_content(input_def.description);
                std::cout << MAA_NS::utf8_to_crt(input_desc) << "\n";
            }
            std::cout << MAA_NS::utf8_to_crt(std::format("{} [{}]: ", input_display_name, default_val));

            std::cin.sync();
            std::string buffer;
            std::getline(std::cin, buffer);

            std::string value = buffer.empty() ? default_val : buffer;

            // Validate with regex if provided
            if (!input_def.verify.empty()) {
                if (auto pattern = MAA_NS::regex_valid(MAA_NS::to_u16(input_def.verify))) {
                    auto value_u16 = MAA_NS::to_u16(value);
                    while (!boost::regex_match(value_u16, *pattern)) {
                        std::string error_msg =
                            input_def.pattern_msg.empty() ? "Invalid input, please retry: " : input_def.pattern_msg + ": ";
                        std::cout << MAA_NS::utf8_to_crt(error_msg);
                        std::getline(std::cin, buffer);
                        value = buffer.empty() ? default_val : buffer;
                        value_u16 = MAA_NS::to_u16(value);
                    }
                }
            }

            config_opt.inputs[input_def.name] = value;
        }
        std::cout << "\n";
    } break;
    }

    config_options.emplace_back(std::move(config_opt));

    // Process nested sub-options if the selected case has any
    if (selected_case && !selected_case->option.empty()) {
        for (const auto& sub_option_name : selected_case->option) {
            if (!process_option(sub_option_name, task_display_name, config_options)) {
                return false;
            }
        }
    }

    return true;
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

        for (const auto& opt : task.option) {
            if (!opt.value.empty()) {
                std::cout << "\t\t- " << MAA_NS::utf8_to_crt(opt.name) << ": " << MAA_NS::utf8_to_crt(opt.value) << "\n";
            }
            else if (!opt.inputs.empty()) {
                std::cout << "\t\t- " << MAA_NS::utf8_to_crt(opt.name) << ":\n";
                for (const auto& [key, val] : opt.inputs) {
                    std::cout << "\t\t\t" << MAA_NS::utf8_to_crt(key) << ": " << MAA_NS::utf8_to_crt(val) << "\n";
                }
            }
        }
    }
    std::cout << "\n";
}

bool Interactor::check_validity()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    if (config_.configuration().controller.type == InterfaceData::Controller::Type::Win32
        && config_.configuration().win32.hwnd == nullptr) {
        auto& name = config_.configuration().controller.name;
        auto controller_iter = std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

        if (controller_iter == config_.interface_data().controller.end()) {
            LogError << "Contorller not found" << VAR(name);
            return false;
        }

        return select_win32_hwnd(controller_iter->win32);
    }

    if (config_.configuration().controller.type == InterfaceData::Controller::Type::PlayCover) {
        if (!kPlayCoverSupported) {
            LogError << "PlayCover controller is only available on macOS";
            return false;
        }

        auto& pc = config_.configuration().playcover;
        if (pc.address.empty()) {
            auto& name = config_.configuration().controller.name;
            auto controller_iter =
                std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

            if (controller_iter != config_.interface_data().controller.end()) {
                select_playcover(controller_iter->playcover);
            }
        }

        if (pc.address.empty()) {
            LogError << "PlayCover address is empty";
            return false;
        }
    }

    if (config_.configuration().controller.type == InterfaceData::Controller::Type::Gamepad) {
        if (!kGamepadSupported) {
            LogError << "Gamepad controller is only available on Windows";
            return false;
        }

        // Gamepad hwnd is optional (for screencap), so no validation needed
        // But we need to select gamepad type if not configured
        if (config_.configuration().gamepad.gamepad_type.empty()) {
            auto& name = config_.configuration().controller.name;
            auto controller_iter =
                std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

            if (controller_iter != config_.interface_data().controller.end()) {
                select_gamepad(controller_iter->gamepad);
            }
        }
    }

    return true;
}

void Interactor::mpause() const
{
    std::cout << "\nPress Enter to continue...";
    std::cin.sync();
    std::cin.get();
}

std::string Interactor::get_display_name(const std::string& name, const std::string& label) const
{
    if (label.empty()) {
        return name;
    }
    // 翻译 label（如果以 $ 开头会被翻译）
    return config_.translate(label);
}

std::string Interactor::read_text_content(const std::string& text) const
{
    if (text.empty()) {
        return {};
    }

    // 先翻译文本（如果以 $ 开头）
    std::string translated = config_.translate(text);

    // 尝试作为文件路径读取
    auto file_path = config_.resource_dir() / MAA_NS::path(translated);
    constexpr size_t kMaxPath = 255;
    if (MAA_NS::path_to_utf8_string(file_path).size() < kMaxPath && std::filesystem::exists(file_path)) {
        std::ifstream ifs(file_path);
        if (ifs.is_open()) {
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            return buffer.str();
        }
    }

    // 不是文件，直接返回翻译后的文本
    return translated;
}
