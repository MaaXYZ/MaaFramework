#include <csignal>
#include <format>
#include <iostream>
#include <vector>

#include "MaaToolkit/MaaToolkitAPI.h"

#include "MaaUtils/Platform.h"
#include "MaaUtils/Runtime.h"

#include "cli_args.h"
#include "interactor.h"
#include "task_config.h"

#if defined(_WIN32)
#include "MaaUtils/SafeWindows.hpp"
#include <shellapi.h>
#endif

void request_windows_emulate_vt100()
{
    // copy from https://github.com/ArthurSonzogni/FTXUI/commit/22afacc28badb35680e7fe03461680c52acbe507
#if defined(_WIN32)
#include "MaaUtils/SafeWindows.hpp"

    // Enable VT processing on stdout and stdin
    auto stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD out_mode = 0;
    GetConsoleMode(stdout_handle, &out_mode);

    // https://docs.microsoft.com/en-us/windows/console/setconsolemode
    const int enable_virtual_terminal_processing = 0x0004;
    const int disable_newline_auto_return = 0x0008;
    out_mode |= enable_virtual_terminal_processing;
    out_mode |= disable_newline_auto_return;

    SetConsoleMode(stdout_handle, out_mode);
#endif
}

extern "C" void sig_handler(int sig)
{
    std::cout << std::format("\nsignal {} received, exit\n", sig) << std::endl;

    exit(0);
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, sig_handler);
    std::signal(SIGSEGV, sig_handler);
    std::signal(SIGINT, sig_handler);
    std::signal(SIGABRT, sig_handler);

    request_windows_emulate_vt100();

#if defined(_WIN32)
    // Windows 下 main 收到的 argv 是 ANSI/OEM (CP936) 编码，而 CLI11 解析后会直接拿去和
    // interface.json (UTF-8) 里的字符串比对，中文参数（如 --resource 官服）会匹配不上。
    // 改用 GetCommandLineW + CommandLineToArgvW 拿到 UTF-16，再统一转 UTF-8 喂给下游。
    int wargc = 0;
    LPWSTR* wargv = ::CommandLineToArgvW(::GetCommandLineW(), &wargc);
    std::vector<std::string> utf8_storage;
    std::vector<char*> utf8_argv;
    if (wargv && wargc > 0) {
        utf8_storage.reserve(static_cast<size_t>(wargc));
        utf8_argv.reserve(static_cast<size_t>(wargc) + 1);
        for (int i = 0; i < wargc; ++i) {
            utf8_storage.emplace_back(MAA_NS::from_osstring(wargv[i]));
            utf8_argv.push_back(utf8_storage.back().data());
        }
        utf8_argv.push_back(nullptr);
        ::LocalFree(wargv);
        argc = wargc;
        argv = utf8_argv.data();
    }
#endif

    auto result = parse_args(argc, argv);
    const auto& args = result.args;

    if (!result.success) {
        std::cerr << "Error: " << result.error_message << "\n\n";
        print_help(result);
        return 1;
    }

    // -h/--help：如果 CLI11 已输出则直接返回，否则打印顶层 help
    if (args.subcommand == Subcommand::PrintTopHelp) {
        if (!result.already_printed) {
            print_help(result);
        }
        return 0;
    }

    // help [tokens...] 子命令：拼接 token 后查表打印对应子命令 help
    if (args.subcommand == Subcommand::HelpCommand) {
        std::string target;
        for (size_t i = 0; i < args.help_tokens.size(); ++i) {
            if (i > 0) {
                target += ' ';
            }
            target += args.help_tokens[i];
        }
        print_help(result, target);
        return 0;
    }

    if (args.subcommand == Subcommand::ShowVersion) {
        std::cout << result.version_message << '\n';
        return 0;
    }

    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());
    std::string resource_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    MaaToolkitConfigInitOption(user_path.c_str(), "{}");

    Interactor interactor(user_path);

    const bool interactive_mode = (args.subcommand == Subcommand::None);
    if (!interactor.load(resource_path, interactive_mode)) {
        return -1;
    }

    // ── list 子命令 ──────────────────────────────────────────────────────────
    if (args.subcommand == Subcommand::List) {
        interactor.list_task();
        return 0;
    }

    // ── generate-config 子命令 ───────────────────────────────────────────────
    if (args.subcommand == Subcommand::GenerateConfigTask) {
        return interactor.generate_task_config(args.force, args.task_output_path, args.task_default_only) ? 0 : -1;
    }

    if (args.subcommand == Subcommand::GenerateConfigPi) {
        return interactor.generate_pi_config(
                   args.controller_name,
                   args.adb_controller_filter,
                   args.resource_name,
                   args.force,
                   args.task_default_only)
                   ? 0
                   : -1;
    }

    // ── run 子命令 ───────────────────────────────────────────────────────────
    if (args.subcommand == Subcommand::Run) {
        if (!args.task_config_path.empty()) {
            // argv 已在入口统一转为 UTF-8，这里不再做 crt_to_utf8 二次转换
            auto task_result = parse_task_config_file(MAA_NS::path(args.task_config_path));
            if (!task_result.success) {
                std::cerr << "Error: " << task_result.error_message << "\n";
                return -1;
            }
            interactor.apply_task_config(task_result.config);
        }
        interactor.print_config();
        return interactor.run(args.progress_level) ? 0 : -1;
    }

    interactor.interact();
    return 0;
}
