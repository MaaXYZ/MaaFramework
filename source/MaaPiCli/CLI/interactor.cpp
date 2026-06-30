#include "interactor.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <boost/regex.hpp>

#include <tomlplusplus/toml.hpp>

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

#if defined(__linux__)
static constexpr bool kWlRootsSupported = true;
#else
static constexpr bool kWlRootsSupported = false;
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

bool Interactor::load(const std::filesystem::path& resource_path, bool interactive)
{
    LogFunc << VAR(resource_path);

    if (!config_.load(resource_path, user_path_)) {
        if (interactive) {
            mpause();
        }
        return false;
    }

    if (!config_.check_configuration()) {
        std::cout << "### The interface has changed and incompatible configurations have been "
                     "deleted. ###\n\n";
        if (interactive) {
            mpause();
        }
    }

    return true;
}

void Interactor::apply_task_config(const TaskConfig& task_cfg)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto& cfg = config_.configuration();

    // Build per-task, per-option lookup from maa_pi_config:
    // saved_opts[task_name][option_name] = Option
    std::unordered_map<std::string, std::unordered_map<std::string, Configuration::Option>> saved_opts;
    for (const auto& saved_task : cfg.task) {
        auto& opt_map = saved_opts[saved_task.name];
        for (const auto& opt : saved_task.option) {
            opt_map.emplace(opt.name, opt);
        }
    }

    // Replace task list with task-config's order/selection.
    cfg.task.clear();
    for (const auto& tc_task : task_cfg.task) {
        // Build a quick lookup of options explicitly provided in task-config.
        std::unordered_map<std::string, Configuration::Option> tc_opts;
        for (const auto& opt : tc_task.option) {
            tc_opts.emplace(opt.name, opt);
        }

        // Determine the full option list for this task from interface data.
        const auto& all_data_tasks = config_.interface_data().task;
        auto data_it = std::ranges::find(all_data_tasks, tc_task.name, std::mem_fn(&InterfaceData::Task::name));

        Configuration::Task resolved_task;
        resolved_task.name = tc_task.name;

        if (data_it == all_data_tasks.end()) {
            // task-config 中的任务名在 interface.json 中找不到：打印 warning 但继续保留，
            // 让后续 check_validity / 执行阶段按既有逻辑处理（与交互式编辑行为一致）。
            LogWarn << "Task in task-config not found in interface.json" << VAR(tc_task.name);
            std::cerr << "Warning: task '" << tc_task.name << "' in task-config not found in interface.json, "
                      << "options (if any) will be ignored.\n";
            // 即使找不到，仍保留显式提供的 options（与 saved 行为对齐），避免静默丢数据
            for (const auto& opt : tc_task.option) {
                resolved_task.option.emplace_back(opt);
            }
            cfg.task.emplace_back(std::move(resolved_task));
            continue;
        }

        auto saved_it = saved_opts.find(tc_task.name);
        const auto* saved_task_opts = saved_it != saved_opts.end() ? &saved_it->second : nullptr;

        for (const auto& opt_name : data_it->option) {
            if (auto it = tc_opts.find(opt_name); it != tc_opts.end()) {
                // 1. task-config explicitly set this option
                resolved_task.option.emplace_back(it->second);
                continue;
            }
            if (saved_task_opts) {
                if (auto it = saved_task_opts->find(opt_name); it != saved_task_opts->end()) {
                    // 2. fall back to saved value from maa_pi_config
                    resolved_task.option.emplace_back(it->second);
                    continue;
                }
            }
            // 3. fall back to interface default
            if (auto def = resolve_option_default(opt_name)) {
                resolved_task.option.emplace_back(std::move(*def));
            }
        }

        // 对 task-config 中显式提供、但当前 task 在 interface 中未声明的 option 名给出 warning
        for (const auto& tc_opt : tc_task.option) {
            if (std::ranges::find(data_it->option, tc_opt.name) == data_it->option.end()) {
                LogWarn << "Option in task-config not declared for this task" << VAR(tc_task.name) << VAR(tc_opt.name);
                std::cerr << "Warning: option '" << tc_opt.name << "' in task-config is not declared by task '" << tc_task.name
                          << "' in interface.json, ignored.\n";
            }
        }

        cfg.task.emplace_back(std::move(resolved_task));
    }

    for (const auto& override_opt : task_cfg.global_option) {
        auto it = std::ranges::find(cfg.global_option, override_opt.name, std::mem_fn(&Configuration::Option::name));
        if (it != cfg.global_option.end()) {
            *it = override_opt;
        }
        else {
            cfg.global_option.emplace_back(override_opt);
        }
    }
}

bool Interactor::resolve_controller(const std::string& controller_name, const std::string& adb_filter)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& controllers = config_.interface_data().controller;
    auto& cfg = config_.configuration();

    if (!controller_name.empty()) {
        auto it = std::ranges::find(controllers, controller_name, std::mem_fn(&InterfaceData::Controller::name));
        if (it == controllers.end()) {
            std::cerr << "Error: Controller '" << MAA_NS::utf8_to_crt(controller_name) << "' not found in interface.json\n";
            std::cerr << "Available controllers:";
            for (const auto& c : controllers) {
                std::cerr << " " << MAA_NS::utf8_to_crt(c.name);
            }
            std::cerr << "\n";
            return false;
        }
        // 显式指定了 controller，但又传了 --adb-controller 且类型不是 Adb：filter 将被忽略
        if (!adb_filter.empty() && it->type != InterfaceData::Controller::Type::Adb) {
            LogWarn << "--adb-controller ignored: selected controller is not Adb" << VAR(controller_name);
            std::cerr << "Warning: --adb-controller is only valid for Adb controller, "
                      << "selected controller '" << MAA_NS::utf8_to_crt(controller_name) << "' is not Adb, filter ignored.\n";
        }
        cfg.controller.name = it->name;
        cfg.controller.type = it->type;
        return true;
    }

    if (!adb_filter.empty()) {
        // --adb-controller implies Adb type; prefer the first Adb controller
        auto it = std::ranges::find(controllers, InterfaceData::Controller::Type::Adb, std::mem_fn(&InterfaceData::Controller::type));
        if (it != controllers.end()) {
            cfg.controller.name = it->name;
            cfg.controller.type = it->type;
            return true;
        }
        // 找不到 Adb controller：与 --adb-controller 的语义冲突，明确告知 filter 将被忽略，
        // 并继续回落到 first controller（保持向后兼容的行为）。
        LogWarn << "--adb-controller specified but no Adb controller in interface.json, falling back to first controller";
        if (!controllers.empty()) {
            std::cerr << "Warning: --adb-controller specified but no Adb controller in interface.json, "
                      << "falling back to '" << controllers.front().name << "' (filter ignored).\n";
        }
        else {
            std::cerr << "Warning: --adb-controller specified but interface.json declares no controller.\n";
        }
    }

    if (!controllers.empty()) {
        cfg.controller.name = controllers.front().name;
        cfg.controller.type = controllers.front().type;
    }
    return true;
}

bool Interactor::find_and_set_adb_device(const std::string& filter)
{
    auto list_handle = MaaToolkitAdbDeviceListCreate();
    OnScopeLeave([&]() { MaaToolkitAdbDeviceListDestroy(list_handle); });
    MaaToolkitAdbDeviceFind(list_handle);

    size_t size = MaaToolkitAdbDeviceListSize(list_handle);
    for (size_t i = 0; i < size; ++i) {
        auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, i);
        std::string name = MaaToolkitAdbDeviceGetName(device_handle);
        std::string path = MaaToolkitAdbDeviceGetAdbPath(device_handle);
        std::string addr = MaaToolkitAdbDeviceGetAddress(device_handle);

        bool matched = filter.empty() || name.find(filter) != std::string::npos || path.find(filter) != std::string::npos
                       || addr.find(filter) != std::string::npos;
        if (!matched) {
            continue;
        }

        auto& adb = config_.configuration().adb;
        adb.name = std::move(name);
        adb.adb_path = std::move(path);
        adb.address = std::move(addr);
        return true;
    }

    if (filter.empty()) {
        std::cerr << "Warning: No ADB devices detected. adb config left empty.\n";
        return true;
    }

    std::cerr << "Error: No ADB device matching '" << filter
              << "' found.\n"
                 "Make sure the device is connected (try: adb connect "
              << filter << ").\n";
    return false;
}

std::optional<MAA_PROJECT_INTERFACE_NS::Configuration::Option> Interactor::resolve_option_default(const std::string& option_name) const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();
    auto opt_it = data.option.find(option_name);
    if (opt_it == data.option.end()) {
        return std::nullopt;
    }
    const auto& opt = opt_it->second;

    Configuration::Option config_opt;
    config_opt.name = option_name;

    bool has_value = std::visit(
        [&](const auto& dc) -> bool {
            using T = std::decay_t<decltype(dc)>;
            if constexpr (std::is_same_v<T, std::string>) {
                if (!dc.empty()) {
                    config_opt.value = dc;
                    return true;
                }
            }
            else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                if (!dc.empty()) {
                    config_opt.values = dc;
                    return true;
                }
            }
            return false;
        },
        opt.default_case);

    if (!has_value) {
        switch (opt.type) {
        case InterfaceData::Option::Type::Select:
        case InterfaceData::Option::Type::Switch:
            if (!opt.cases.empty()) {
                config_opt.value = opt.cases.front().name;
                has_value = true;
            }
            break;
        case InterfaceData::Option::Type::Checkbox:
            // checkbox has no default when empty (do not force-select the first item)
            break;
        case InterfaceData::Option::Type::Input:
            for (const auto& input_def : opt.inputs) {
                config_opt.inputs[input_def.name] = input_def.default_;
            }
            has_value = !opt.inputs.empty();
            break;
        }
    }

    if (!has_value) {
        return std::nullopt;
    }
    return config_opt;
}

std::vector<const MAA_PROJECT_INTERFACE_NS::InterfaceData::Task*> Interactor::filter_tasks(bool default_only, bool filter_env) const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();
    const auto& cfg = config_.configuration();

    std::vector<const InterfaceData::Task*> result;
    for (const auto& task : data.task) {
        if (default_only && !task.default_check) {
            continue;
        }
        if (filter_env) {
            if (!task.resource.empty() && std::ranges::find(task.resource, cfg.resource) == task.resource.end()) {
                continue;
            }
            if (!task.controller.empty() && std::ranges::find(task.controller, cfg.controller.name) == task.controller.end()) {
                continue;
            }
        }
        result.push_back(&task);
    }
    return result;
}

void Interactor::fill_default_tasks(bool default_only)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();
    auto& cfg = config_.configuration();

    for (const auto* task_ptr : filter_tasks(default_only, true)) {
        const auto& task = *task_ptr;

        Configuration::Task config_task;
        config_task.name = task.name;

        for (const auto& option_name : task.option) {
            auto opt_it = data.option.find(option_name);
            if (opt_it == data.option.end()) {
                continue;
            }
            const auto& opt = opt_it->second;

            if (!opt.controller.empty() && std::ranges::find(opt.controller, cfg.controller.name) == opt.controller.end()) {
                continue;
            }
            if (!opt.resource.empty() && std::ranges::find(opt.resource, cfg.resource) == opt.resource.end()) {
                continue;
            }

            auto config_opt = resolve_option_default(option_name);
            if (config_opt) {
                config_task.option.emplace_back(std::move(*config_opt));
            }
        }

        cfg.task.emplace_back(std::move(config_task));
        std::cout << "Added task: " << MAA_NS::utf8_to_crt(get_display_name(task.name, task.label)) << "\n";
    }

    if (!cfg.task.empty()) {
        std::cout << "\n";
    }
}

bool Interactor::resolve_resource(const std::string& resource_name)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& resources = config_.interface_data().resource;
    auto& cfg = config_.configuration();

    if (!resource_name.empty()) {
        auto it = std::ranges::find(resources, resource_name, std::mem_fn(&InterfaceData::Resource::name));
        if (it == resources.end()) {
            std::cerr << "Error: Resource '" << MAA_NS::utf8_to_crt(resource_name) << "' not found in interface.json\n";
            std::cerr << "Available resources:";
            for (const auto& r : resources) {
                std::cerr << " " << MAA_NS::utf8_to_crt(r.name);
            }
            std::cerr << "\n";
            return false;
        }
        cfg.resource = it->name;
        return true;
    }

    if (!resources.empty()) {
        cfg.resource = resources.front().name;
    }
    return true;
}

bool Interactor::generate_pi_config(
    const std::string& controller_name,
    const std::string& adb_filter,
    const std::string& resource_name,
    bool force,
    bool default_only)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto config_path = user_path_ / "config" / "maa_pi_config.json";
    if (!force && std::filesystem::exists(config_path)) {
        std::cerr << "Error: " << config_path.string() << " already exists. Use --force to overwrite.\n";
        return false;
    }

    auto& cfg = config_.configuration();

    if (!resolve_controller(controller_name, adb_filter)) {
        return false;
    }

    if (!resolve_resource(resource_name)) {
        return false;
    }

    if (cfg.controller.type == InterfaceData::Controller::Type::Adb) {
        if (!find_and_set_adb_device(adb_filter)) {
            return false;
        }
    }
    else if (!adb_filter.empty()) {
        std::cerr << "Warning: --adb-controller is only valid for Adb controller type, ignored.\n";
    }

    // Always repopulate tasks from defaults; do not carry over stale config
    cfg.task.clear();
    fill_default_tasks(default_only);

    config_.save(user_path_);

    std::cout << "Generated config/maa_pi_config.json"
              << " (controller: " << MAA_NS::utf8_to_crt(cfg.controller.name);
    if (!cfg.adb.address.empty()) {
        std::cout << ", adb: " << MAA_NS::utf8_to_crt(cfg.adb.address);
    }
    if (!cfg.resource.empty()) {
        std::cout << ", resource: " << MAA_NS::utf8_to_crt(cfg.resource);
    }
    std::cout << ")\n";
    return true;
}

bool Interactor::generate_task_config(bool force, const std::string& output_path, bool default_only) const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    static constexpr const char* kDefaultFileName = "sample_task_config.toml";
    // output_path 来自 CLI argv，已在 main 入口统一转为 UTF-8，此处直接用 MAA_NS::path
    const std::filesystem::path target = output_path.empty() ? std::filesystem::path(kDefaultFileName) : MAA_NS::path(output_path);
    const std::string target_display = MAA_NS::utf8_to_crt(MAA_NS::path_to_utf8_string(target));

    if (!force && std::filesystem::exists(target)) {
        std::cerr << "Error: " << target_display << " already exists. Use --force to overwrite.\n";
        return false;
    }

    const auto& data = config_.interface_data();

    // Build a toml inline table node for one option entry
    auto make_option_node = [&](const Configuration::Option& opt) -> toml::table {
        toml::table tbl;
        tbl.emplace("name", opt.name);
        if (!opt.value.empty()) {
            tbl.emplace("value", opt.value);
        }
        else if (!opt.values.empty()) {
            toml::array arr;
            for (const auto& v : opt.values) {
                arr.push_back(v);
            }
            tbl.emplace("values", std::move(arr));
        }
        else if (!opt.inputs.empty()) {
            toml::table inputs_tbl;
            for (const auto& [k, v] : opt.inputs) {
                inputs_tbl.emplace(k, v);
            }
            tbl.emplace("inputs", std::move(inputs_tbl));
        }
        tbl.is_inline(true);
        return tbl;
    };

    toml::table root;

    toml::array task_arr;
    size_t written = 0;
    for (const auto* task_ptr : filter_tasks(default_only, false)) {
        const auto& task = *task_ptr;
        toml::table task_tbl;
        task_tbl.emplace("name", task.name);

        toml::array opt_arr;
        for (const auto& option_name : task.option) {
            auto resolved = resolve_option_default(option_name);
            if (resolved) {
                opt_arr.push_back(make_option_node(*resolved));
            }
        }
        if (!opt_arr.empty()) {
            task_tbl.emplace("option", std::move(opt_arr));
        }

        task_arr.push_back(std::move(task_tbl));
        ++written;
    }
    root.emplace("task", std::move(task_arr));

    if (!data.global_option.empty()) {
        toml::array global_arr;
        for (const auto& option_name : data.global_option) {
            auto resolved = resolve_option_default(option_name);
            if (resolved) {
                global_arr.push_back(make_option_node(*resolved));
            }
        }
        if (!global_arr.empty()) {
            root.emplace("global_option", std::move(global_arr));
        }
    }

    std::ofstream ofs(target);
    if (!ofs.is_open()) {
        std::cerr << "Error: Failed to create " << target_display << "\n";
        return false;
    }

    ofs << "# Sample task config file for MaaPiCli\n"
           "# Use with: MaaPiCli run --task-config "
        << target_display
        << "\n"
           "#\n"
           "# This file specifies which tasks to run and any temporary option overrides.\n"
           "# Device/controller/resource configuration comes from maa_pi_config.json\n"
           "# (loaded via -d or --config).\n"
           "#\n"
           "# Generated from interface.json - edit as needed.\n"
           "\n";
    ofs << toml::default_formatter { root };

    std::cout << "Generated: " << target_display << " (" << written << " tasks)\n";
    return true;
}

void Interactor::list_task() const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();

    std::cout << "### Available Controllers ###\n\n";
    for (const auto& ctrl : data.controller) {
        std::string display_name = get_display_name(ctrl.name, ctrl.label);
        std::string type_str;
        switch (ctrl.type) {
        case InterfaceData::Controller::Type::Adb:
            type_str = "Adb";
            break;
        case InterfaceData::Controller::Type::Win32:
            type_str = "Win32";
            break;
        case InterfaceData::Controller::Type::MacOS:
            type_str = "MacOS";
            break;
        case InterfaceData::Controller::Type::PlayCover:
            type_str = "PlayCover";
            break;
        case InterfaceData::Controller::Type::Gamepad:
            type_str = "Gamepad";
            break;
        case InterfaceData::Controller::Type::WlRoots:
            type_str = "WlRoots";
            break;
        default:
            type_str = "Unknown";
            break;
        }

        std::cout << MAA_NS::utf8_to_crt(std::format("  - {} [{}]\n", display_name, type_str));
        if (!ctrl.description.empty()) {
            std::string desc = read_text_content(ctrl.description);
            std::cout << MAA_NS::utf8_to_crt(std::format("    {}\n", desc));
        }
    }
    std::cout << "\n";

    std::cout << "### Available Tasks ###\n\n";
    for (const auto& task : data.task) {
        std::string display_name = get_display_name(task.name, task.label);
        std::cout << MAA_NS::utf8_to_crt(std::format("  - {}\n", display_name));
        if (!task.description.empty()) {
            std::string desc = read_text_content(task.description);
            std::cout << MAA_NS::utf8_to_crt(std::format("    {}\n", desc));
        }

        for (const auto& opt_name : task.option) {
            auto opt_it = data.option.find(opt_name);
            if (opt_it == data.option.end()) {
                continue;
            }
            const auto& opt = opt_it->second;
            std::string opt_display = get_display_name(opt_name, opt.label);

            std::string type_str;
            switch (opt.type) {
            case InterfaceData::Option::Type::Select:
                type_str = "select";
                break;
            case InterfaceData::Option::Type::Switch:
                type_str = "switch";
                break;
            case InterfaceData::Option::Type::Checkbox:
                type_str = "checkbox";
                break;
            case InterfaceData::Option::Type::Input:
                type_str = "input";
                break;
            }

            std::cout << MAA_NS::utf8_to_crt(std::format("    Option: {} ({})\n", opt_display, type_str));

            if (opt.type == InterfaceData::Option::Type::Input) {
                for (const auto& input_def : opt.inputs) {
                    std::string input_display = get_display_name(input_def.name, input_def.label);
                    std::string default_val = input_def.default_.empty() ? "" : std::format(" [default: {}]", input_def.default_);
                    std::cout << MAA_NS::utf8_to_crt(std::format("      - {}{}\n", input_display, default_val));
                }
            }
            else {
                for (const auto& case_item : opt.cases) {
                    std::string case_display = get_display_name(case_item.name, case_item.label);
                    std::cout << MAA_NS::utf8_to_crt(std::format("      - {}\n", case_display));
                }
            }
        }
    }
    std::cout << "\n";

    if (!data.global_option.empty()) {
        std::cout << "### Global Options ###\n\n";
        for (const auto& opt_name : data.global_option) {
            auto opt_it = data.option.find(opt_name);
            if (opt_it == data.option.end()) {
                continue;
            }
            const auto& opt = opt_it->second;
            std::string opt_display = get_display_name(opt_name, opt.label);

            std::string type_str;
            switch (opt.type) {
            case InterfaceData::Option::Type::Select:
                type_str = "select";
                break;
            case InterfaceData::Option::Type::Switch:
                type_str = "switch";
                break;
            case InterfaceData::Option::Type::Checkbox:
                type_str = "checkbox";
                break;
            case InterfaceData::Option::Type::Input:
                type_str = "input";
                break;
            }

            std::cout << MAA_NS::utf8_to_crt(std::format("  - {} ({})\n", opt_display, type_str));

            if (opt.type == InterfaceData::Option::Type::Input) {
                for (const auto& input_def : opt.inputs) {
                    std::string input_display = get_display_name(input_def.name, input_def.label);
                    std::string default_val = input_def.default_.empty() ? "" : std::format(" [default: {}]", input_def.default_);
                    std::cout << MAA_NS::utf8_to_crt(std::format("    - {}{}\n", input_display, default_val));
                }
            }
            else {
                for (const auto& case_item : opt.cases) {
                    std::string case_display = get_display_name(case_item.name, case_item.label);
                    std::cout << MAA_NS::utf8_to_crt(std::format("    - {}\n", case_display));
                }
            }
        }
        std::cout << "\n";
    }
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

bool Interactor::run(int progress_level)
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

    bool ret = MAA_PROJECT_INTERFACE_NS::Runner::run(runtime.value(), progress_level);

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
    case InterfaceData::Controller::Type::MacOS: {
        const auto& macos = config_.configuration().macos;
        std::cout << MAA_NS::utf8_to_crt(
            std::format(
                "\t\tWindow ID: {}\n\t\tTitle: {}\n\t\tScreencap: {}\n\t\tInput: {}\n",
                macos.window_id,
                macos.title,
                macos.screencap,
                macos.input));
    } break;
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
    case InterfaceData::Controller::Type::WlRoots: {
        const auto& wlr = config_.configuration().wlroots;
        if (!wlr.wlr_socket_path.empty()) {
            std::cout << MAA_NS::utf8_to_crt(std::format("\t\t{}\n", wlr.wlr_socket_path));
        }
        if (!kWlRootsSupported) {
            std::cout << "\t\t(WLRoots is only available on Linux)\n";
        }
    } break;
    default:
        LogError << "Unknown controller type" << VAR(config_.configuration().controller.type);
        break;
    }

    std::cout << "\n";

    std::cout << "Resource:\n\n";
    std::cout << "\t" << MAA_NS::utf8_to_crt(config_.configuration().resource) << "\n\n";

    auto print_level_options = [&](const std::string& label, const std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option>& opts) {
        if (opts.empty()) {
            return;
        }
        std::cout << label << ":\n\n";
        for (const auto& opt : opts) {
            if (!opt.value.empty()) {
                std::cout << "\t" << MAA_NS::utf8_to_crt(opt.name) << ": " << MAA_NS::utf8_to_crt(opt.value) << "\n";
            }
            else if (!opt.values.empty()) {
                std::cout << "\t" << MAA_NS::utf8_to_crt(opt.name) << ": [";
                for (size_t j = 0; j < opt.values.size(); ++j) {
                    if (j > 0) {
                        std::cout << ", ";
                    }
                    std::cout << MAA_NS::utf8_to_crt(opt.values[j]);
                }
                std::cout << "]\n";
            }
            else if (!opt.inputs.empty()) {
                std::cout << "\t" << MAA_NS::utf8_to_crt(opt.name) << ":\n";
                for (const auto& [key, val] : opt.inputs) {
                    std::cout << "\t\t" << MAA_NS::utf8_to_crt(key) << ": " << MAA_NS::utf8_to_crt(val) << "\n";
                }
            }
        }
        std::cout << "\n";
    };

    print_level_options("Global Options", config_.configuration().global_option);
    print_level_options("Resource Options", config_.configuration().resource_option);
    print_level_options("Controller Options", config_.configuration().controller_option);

    std::cout << "Tasks:\n\n";
    print_config_tasks(false);
}

void Interactor::interact_for_first_time_use()
{
    welcome();
    select_controller();
    select_resource();

    // v2.3.0: process global/resource/controller-level options
    process_level_options(config_.interface_data().global_option, config_.configuration().global_option, "Global");

    if (auto res_it = std::ranges::find(
            config_.interface_data().resource,
            config_.configuration().resource,
            std::mem_fn(&MAA_PROJECT_INTERFACE_NS::InterfaceData::Resource::name));
        res_it != config_.interface_data().resource.end()) {
        process_level_options(res_it->option, config_.configuration().resource_option, "Resource");
    }

    if (auto ctrl_it = std::ranges::find(
            config_.interface_data().controller,
            config_.configuration().controller.name,
            std::mem_fn(&MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::name));
        ctrl_it != config_.interface_data().controller.end()) {
        process_level_options(ctrl_it->option, config_.configuration().controller_option, "Controller");
    }

    add_default_tasks();

    if (config_.configuration().task.empty()) {
        add_task();
    }
}

void Interactor::welcome() const
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& data = config_.interface_data();

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

    if (!data.welcome.empty()) {
        std::string welcome_text = read_text_content(data.welcome);
        std::cout << MAA_NS::utf8_to_crt(welcome_text) << "\n\n";
    }

    if (!data.description.empty()) {
        std::string desc_text = read_text_content(data.description);
        std::cout << "Description: " << MAA_NS::utf8_to_crt(desc_text) << "\n\n";
    }

    if (!data.github.empty()) {
        std::cout << "GitHub: " << MAA_NS::utf8_to_crt(data.github) << "\n\n";
    }

    if (!data.contact.empty()) {
        std::string contact_text = read_text_content(data.contact);
        std::cout << "Contact: " << MAA_NS::utf8_to_crt(contact_text) << "\n\n";
    }

    if (!data.license.empty()) {
        std::string license_text = read_text_content(data.license);
        std::cout << "License: " << MAA_NS::utf8_to_crt(license_text) << "\n\n";
    }
}

bool Interactor::interact_once()
{
    bool has_presets = !config_.interface_data().preset.empty();

    std::cout << "### Select action ###\n\n";
    std::cout << "\t1. Switch controller\n";
    std::cout << "\t2. Switch resource\n";
    std::cout << "\t3. Add task\n";
    std::cout << "\t4. Move task\n";
    std::cout << "\t5. Delete task\n";
    std::cout << "\t6. Run tasks\n";
    if (has_presets) {
        std::cout << "\t7. Apply preset\n";
        std::cout << "\t8. Exit\n";
    }
    else {
        std::cout << "\t7. Exit\n";
    }
    std::cout << "\n";

    int max_action = has_presets ? 8 : 7;
    int action = input(max_action);
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
        if (has_presets) {
            apply_preset();
        }
        else {
            return false;
        }
        break;
    case 8:
        if (has_presets) {
            return false;
        }
        break;
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
    case InterfaceData::Controller::Type::MacOS:
        config_.configuration().controller.type = InterfaceData::Controller::Type::MacOS;
        select_macos(controller.macos);
        break;
    case InterfaceData::Controller::Type::PlayCover:
        if (!kPlayCoverSupported) {
            std::cout << "\nPlayCover controller is only available on macOS.\n";
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
    case InterfaceData::Controller::Type::WlRoots:
        if (!kWlRootsSupported) {
            std::cout << "\nWlRoots controller is only available on Linux.\n";
            bool has_other_controllers = std::ranges::any_of(all_controllers, [](const auto& ctrl) {
                return ctrl.type != InterfaceData::Controller::Type::WlRoots;
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
        config_.configuration().controller.type = InterfaceData::Controller::Type::WlRoots;
        select_wlroots();
        break;
    case InterfaceData::Controller::Type::Gamepad:
        if (!kGamepadSupported) {
            std::cout << "\nGamepad controller is only available on Windows.\n";
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

    if (!playcover_config.uuid.empty() && pc.uuid.empty()) {
        pc.uuid = playcover_config.uuid;
    }

    std::string default_address = pc.address.empty() ? "127.0.0.1:1717" : pc.address;

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

    std::cout << "\n### Select Gamepad Type ###\n\n";
    std::cout << "\t1. Xbox 360\n";
    std::cout << "\t2. DualShock 4 (PS4)\n";
    std::cout << "\n";

    int type_index = input(2);
    config_.configuration().gamepad.gamepad_type = (type_index == 1) ? "Xbox360" : "DualShock4";

    std::cout << "\n";
}

void Interactor::select_macos(const MAA_PROJECT_INTERFACE_NS::InterfaceData::Controller::MacOSConfig& macos_config)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    std::cout << "### Configure macOS Controller ###\n\n";

    auto& mac = config_.configuration().macos;

    std::string title_regex_str = macos_config.title_regex;
    if (title_regex_str.empty()) {
        std::cout << "Title regex: ";
        std::cin.sync();
        std::getline(std::cin, title_regex_str);

        if (std::cin.eof()) {
            s_eof = true;
            return;
        }
    }

    if (!title_regex_str.empty()) {
        auto title_regex = MAA_NS::regex_valid(MAA_NS::to_u16(title_regex_str));
        if (title_regex) {
            auto list_handle = MaaToolkitDesktopWindowListCreate();
            OnScopeLeave([&]() { MaaToolkitDesktopWindowListDestroy(list_handle); });

            MaaToolkitDesktopWindowFindAll(list_handle);

            size_t list_size = MaaToolkitDesktopWindowListSize(list_handle);

            std::vector<std::pair<uint32_t, std::string>> matched_windows;
            for (size_t i = 0; i < list_size; ++i) {
                auto window_handle = MaaToolkitDesktopWindowListAt(list_handle, i);
                std::string window_name = MaaToolkitDesktopWindowGetWindowName(window_handle);

                if (boost::regex_search(MAA_NS::to_u16(window_name), *title_regex)) {
                    void* hwnd = MaaToolkitDesktopWindowGetHandle(window_handle);
                    // On macOS, hwnd is (void*)(uintptr_t)windowID, so extract uint32_t
                    uint32_t window_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hwnd));
                    matched_windows.emplace_back(window_id, window_name);
                }
            }

            if (!matched_windows.empty()) {
                if (matched_windows.size() == 1) {
                    mac.window_id = matched_windows.front().first;
                    mac.title = matched_windows.front().second;
                    std::cout << "Auto-selected window ID: " << mac.window_id << "\n\n";
                }
                else {
                    std::cout << "### Select Window ###\n\n";
                    for (size_t i = 0; i < matched_windows.size(); ++i) {
                        std::cout << MAA_NS::utf8_to_crt(
                            std::format("\t{}. Window ID: {} - {}\n", i + 1, matched_windows[i].first, matched_windows[i].second));
                    }
                    std::cout << "\n";

                    int index = input(matched_windows.size()) - 1;
                    mac.window_id = matched_windows[index].first;
                    mac.title = matched_windows[index].second;
                }
            }
            else {
                LogWarn << "No window matched regex" << VAR(title_regex_str);
                std::cout << "No window found matching regex.\n\n";
                return;
            }
        }
        else {
            LogError << "Invalid title regex" << VAR(title_regex_str);
            return;
        }
    }
    else {
        std::cout << "Title regex is required.\n\n";
        return;
    }

    if (!macos_config.screencap.empty() && mac.screencap.empty()) {
        mac.screencap = macos_config.screencap;
    }

    if (!macos_config.input.empty() && mac.input.empty()) {
        mac.input = macos_config.input;
    }

    std::string default_screencap = mac.screencap.empty() ? "ScreenCaptureKit" : mac.screencap;
    std::string default_input = mac.input.empty() ? "GlobalEvent" : mac.input;

    std::cout << "Screencap method [" << default_screencap << "]: ";
    std::cin.sync();
    std::string buffer;
    std::getline(std::cin, buffer);

    if (std::cin.eof()) {
        s_eof = true;
        return;
    }

    mac.screencap = buffer.empty() ? default_screencap : buffer;

    std::cout << "Input method [" << default_input << "]: ";
    std::cin.sync();
    std::getline(std::cin, buffer);

    if (std::cin.eof()) {
        s_eof = true;
        return;
    }

    mac.input = buffer.empty() ? default_input : buffer;
    std::cout << "\n";
}

void Interactor::select_wlroots()
{
    std::cout << "### Select Wayland Socket ###\n\n";

    std::cout << "\t1. Auto detect\n";
    std::cout << "\t2. Manual input\n";
    std::cout << "\n";

    int action = input(2);

    switch (action) {
    case 1:
        select_wlroots_auto_detect();
        break;

    case 2:
        select_wlroots_manual_input();
        break;
    }
}

void Interactor::select_wlroots_auto_detect()
{
    std::cout << "Finding sockets...\n\n";

    auto list_handle = MaaToolkitDesktopWindowListCreate();
    OnScopeLeave([&]() { MaaToolkitDesktopWindowListDestroy(list_handle); });

    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t size = MaaToolkitDesktopWindowListSize(list_handle);
    if (size == 0) {
        std::cout << "No sockets found!\n\n";
        select_wlroots();
        return;
    }

    std::cout << "## Select Socket ##\n\n";

    for (size_t i = 0; i < size; ++i) {
        auto compositor = MaaToolkitDesktopWindowListAt(list_handle, i);

        auto id = MaaToolkitDesktopWindowGetHandle(compositor);
        std::string name = MaaToolkitDesktopWindowGetWindowName(compositor);
        std::string path = MaaToolkitDesktopWindowGetClassName(compositor);

        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n\t\t{}\n\t\t{}\n", i + 1, id, name, path));
    }
    std::cout << "\n";

    int index = input(size) - 1;
    auto& wlr_config = config_.configuration().wlroots;

    auto compositor = MaaToolkitDesktopWindowListAt(list_handle, index);

    wlr_config.wlr_socket_path = MaaToolkitDesktopWindowGetClassName(compositor);
}

void Interactor::select_wlroots_manual_input()
{
    std::cout << "Please input Wayland socket path: ";
    std::cin.sync();
    std::string socket_path;
    std::getline(std::cin, socket_path);
    config_.configuration().wlroots.wlr_socket_path = socket_path;
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

    std::vector<const InterfaceData::Resource*> available_resources;
    for (const auto& res : all_resources) {
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

    if (config_.interface_data().task.empty()) {
        LogError << "Task is empty";
        return;
    }

    auto available_tasks = filter_tasks(false, true);

    if (available_tasks.empty()) {
        const auto& current_resource = config_.configuration().resource;
        const auto& current_controller = config_.configuration().controller.name;
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

    for (const auto* task_ptr : filter_tasks(true, true)) {
        const auto& task = *task_ptr;
        std::string task_display_name = get_display_name(task.name, task.label);

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

    // v2.3.1: check option applicability
    if (!opt.controller.empty() && std::ranges::find(opt.controller, config_.configuration().controller.name) == opt.controller.end()) {
        return true;
    }
    if (!opt.resource.empty() && std::ranges::find(opt.resource, config_.configuration().resource) == opt.resource.end()) {
        return true;
    }

    Configuration::Option config_opt;
    config_opt.name = option_name;

    std::string opt_display_name = get_display_name(option_name, opt.label);

    std::vector<const InterfaceData::Option::Case*> selected_cases;

    switch (opt.type) {
    case InterfaceData::Option::Type::Select: {
        if (auto* str = std::get_if<std::string>(&opt.default_case); str && !str->empty()) {
            config_opt.value = *str;
            auto case_iter = std::ranges::find(opt.cases, *str, std::mem_fn(&InterfaceData::Option::Case::name));
            if (case_iter != opt.cases.end()) {
                selected_cases.push_back(&(*case_iter));
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
            selected_cases.push_back(&opt.cases[case_index]);
        }
    } break;

    case InterfaceData::Option::Type::Switch: {
        // Switch type must have exactly two cases
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

        const auto* matched_case = find_case(is_yes);
        selected_cases.push_back(matched_case);
        config_opt.value = matched_case->name;
        std::cout << "\n";
    } break;

    case InterfaceData::Option::Type::Checkbox: {
        if (auto* vec = std::get_if<std::vector<std::string>>(&opt.default_case); vec && !vec->empty()) {
            config_opt.values = *vec;
        }
        else {
            std::cout << MAA_NS::utf8_to_crt(
                std::format("\n\n## Checkbox option \"{}\" for \"{}\" (multi-select) ##\n\n", opt_display_name, task_display_name));
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

            auto indexes = input_multi(opt.cases.size());
            for (int idx : indexes) {
                config_opt.values.emplace_back(opt.cases[idx - 1].name);
            }
        }

        for (const auto& val : config_opt.values) {
            auto it = std::ranges::find(opt.cases, val, std::mem_fn(&InterfaceData::Option::Case::name));
            if (it != opt.cases.end()) {
                selected_cases.push_back(&(*it));
            }
        }
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

    for (const auto* sc : selected_cases) {
        for (const auto& sub_option_name : sc->option) {
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
    using namespace MAA_PROJECT_INTERFACE_NS;

    auto& all_config_tasks = config_.configuration().task;
    const auto& groups = config_.interface_data().group;

    auto print_task_options = [&](const Configuration::Task& task) {
        for (const auto& opt : task.option) {
            if (!opt.value.empty()) {
                std::cout << "\t\t- " << MAA_NS::utf8_to_crt(opt.name) << ": " << MAA_NS::utf8_to_crt(opt.value) << "\n";
            }
            else if (!opt.values.empty()) {
                std::cout << "\t\t- " << MAA_NS::utf8_to_crt(opt.name) << ": [";
                for (size_t j = 0; j < opt.values.size(); ++j) {
                    if (j > 0) {
                        std::cout << ", ";
                    }
                    std::cout << MAA_NS::utf8_to_crt(opt.values[j]);
                }
                std::cout << "]\n";
            }
            else if (!opt.inputs.empty()) {
                std::cout << "\t\t- " << MAA_NS::utf8_to_crt(opt.name) << ":\n";
                for (const auto& [key, val] : opt.inputs) {
                    std::cout << "\t\t\t" << MAA_NS::utf8_to_crt(key) << ": " << MAA_NS::utf8_to_crt(val) << "\n";
                }
            }
        }
    };

    // v2.4.0: display tasks grouped if groups are defined
    if (!groups.empty() && !with_index) {
        std::unordered_set<std::string> printed_tasks;

        for (const auto& grp : groups) {
            std::string grp_display = get_display_name(grp.name, grp.label);
            bool has_task_in_group = false;
            for (const auto& cfg_task : all_config_tasks) {
                auto data_it = std::ranges::find(config_.interface_data().task, cfg_task.name, std::mem_fn(&InterfaceData::Task::name));
                if (data_it == config_.interface_data().task.end()) {
                    continue;
                }
                if (std::ranges::find(data_it->group, grp.name) == data_it->group.end()) {
                    continue;
                }
                has_task_in_group = true;
                break;
            }
            if (!has_task_in_group) {
                continue;
            }

            std::cout << "  [" << MAA_NS::utf8_to_crt(grp_display) << "]\n";
            for (const auto& cfg_task : all_config_tasks) {
                auto data_it = std::ranges::find(config_.interface_data().task, cfg_task.name, std::mem_fn(&InterfaceData::Task::name));
                if (data_it == config_.interface_data().task.end()) {
                    continue;
                }
                if (std::ranges::find(data_it->group, grp.name) == data_it->group.end()) {
                    continue;
                }
                std::cout << MAA_NS::utf8_to_crt(std::format("\t- {}\n", cfg_task.name));
                print_task_options(cfg_task);
                printed_tasks.insert(cfg_task.name);
            }
        }

        bool has_ungrouped = false;
        for (const auto& cfg_task : all_config_tasks) {
            if (printed_tasks.contains(cfg_task.name)) {
                continue;
            }
            if (!has_ungrouped) {
                std::cout << "  [Other]\n";
                has_ungrouped = true;
            }
            std::cout << MAA_NS::utf8_to_crt(std::format("\t- {}\n", cfg_task.name));
            print_task_options(cfg_task);
        }
    }
    else {
        for (size_t i = 0; i < all_config_tasks.size(); ++i) {
            const auto& task = all_config_tasks[i];
            if (with_index) {
                std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, task.name));
            }
            else {
                std::cout << MAA_NS::utf8_to_crt(std::format("\t- {}\n", task.name));
            }
            print_task_options(task);
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

    if (config_.configuration().controller.type == InterfaceData::Controller::Type::MacOS) {
        auto& mac = config_.configuration().macos;
        if (mac.window_id == 0 || mac.title.empty()) {
            auto& name = config_.configuration().controller.name;
            auto controller_iter =
                std::ranges::find(config_.interface_data().controller, name, std::mem_fn(&InterfaceData::Controller::name));

            if (controller_iter == config_.interface_data().controller.end()) {
                LogError << "Controller not found" << VAR(name);
                return false;
            }

            select_macos(controller_iter->macos);
            return mac.window_id != 0;
        }
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

        // hwnd is optional for screencap; only ensure gamepad type is configured
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

void Interactor::process_level_options(
    const std::vector<std::string>& option_names,
    std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option>& config_options,
    const std::string& level_label)
{
    if (option_names.empty()) {
        return;
    }

    config_options.clear();
    for (const auto& option_name : option_names) {
        if (!process_option(option_name, level_label, config_options)) {
            LogWarn << "Failed to process" << level_label << "option" << VAR(option_name);
        }
    }
}

void Interactor::apply_preset()
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    const auto& presets = config_.interface_data().preset;
    if (presets.empty()) {
        std::cout << "No presets available.\n\n";
        return;
    }

    std::cout << "### Apply preset ###\n\n";
    for (size_t i = 0; i < presets.size(); ++i) {
        const auto& p = presets[i];
        std::string display_name = get_display_name(p.name, p.label);
        std::cout << MAA_NS::utf8_to_crt(std::format("\t{}. {}\n", i + 1, display_name));
        if (!p.description.empty()) {
            std::string desc_text = read_text_content(p.description);
            std::cout << "\t   " << MAA_NS::utf8_to_crt(desc_text) << "\n";
        }
    }
    std::cout << "\n";

    int index = input(presets.size()) - 1;
    const auto& preset = presets[index];

    config_.configuration().task.clear();
    for (const auto& preset_task : preset.task) {
        if (!preset_task.enabled) {
            continue;
        }

        auto data_iter = std::ranges::find(config_.interface_data().task, preset_task.name, std::mem_fn(&InterfaceData::Task::name));
        if (data_iter == config_.interface_data().task.end()) {
            LogWarn << "Preset references unknown task" << VAR(preset_task.name);
            continue;
        }

        Configuration::Task config_task;
        config_task.name = preset_task.name;

        for (const auto& [opt_name, opt_value] : preset_task.option) {
            auto opt_iter = config_.interface_data().option.find(opt_name);
            if (opt_iter == config_.interface_data().option.end()) {
                continue;
            }

            Configuration::Option config_opt;
            config_opt.name = opt_name;

            switch (opt_iter->second.type) {
            case InterfaceData::Option::Type::Select:
            case InterfaceData::Option::Type::Switch:
                if (opt_value.is_string()) {
                    config_opt.value = opt_value.as_string();
                }
                break;
            case InterfaceData::Option::Type::Checkbox:
                if (opt_value.is_array()) {
                    for (const auto& v : opt_value.as_array()) {
                        if (v.is_string()) {
                            config_opt.values.emplace_back(v.as_string());
                        }
                    }
                }
                break;
            case InterfaceData::Option::Type::Input:
                if (opt_value.is_object()) {
                    for (const auto& [k, v] : opt_value.as_object()) {
                        if (v.is_string()) {
                            config_opt.inputs[k] = v.as_string();
                        }
                    }
                }
                break;
            }

            config_task.option.emplace_back(std::move(config_opt));
        }

        config_.configuration().task.emplace_back(std::move(config_task));
    }

    std::string preset_display = get_display_name(preset.name, preset.label);
    std::cout << "Applied preset: " << MAA_NS::utf8_to_crt(preset_display) << "\n\n";
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
