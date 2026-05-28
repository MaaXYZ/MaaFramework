#include "cli_args.h"

#include <iostream>

#include <CLI/CLI11.hpp>

ParseResult parse_args(int argc, char** argv)
{
    ParseResult result;
    auto& args = result.args;

    CLI::App app { "MaaPiCli - MAA Project Interface CLI" };
    app.set_version_flag("-v,--version", "MaaPiCli");
    app.allow_extras(false);

    // ── run 子命令 ──────────────────────────────────────────────────────────
    auto* sub_run = app.add_subcommand(std::string(SubcommandName::Run), "Run tasks; optionally override with a task config file");
    sub_run->add_option(
        "--task-config",
        args.task_config_path,
        "Path to task config file (TOML/JSON); if omitted, runs saved config directly");
    sub_run->add_option("--progress-level", args.progress_level, "Progress output level: 0=silent, 1=task (default), 2=task+node")
        ->check(CLI::Range(0, 2));

    // ── list 子命令 ─────────────────────────────────────────────────────────
    auto* sub_list = app.add_subcommand(std::string(SubcommandName::List), "List available controllers, tasks and options");

    // ── generate-config 父子命令 ────────────────────────────────────────────
    auto* sub_gen = app.add_subcommand(std::string(SubcommandName::GenerateConfig), "Generate config files");
    sub_gen->require_subcommand(1);

    auto* sub_gen_pi = sub_gen->add_subcommand("pi", "Generate config/maa_pi_config.json");
    sub_gen_pi->add_option("--controller", args.controller_name, "Controller name from interface.json (default: first, usually Android)");
    sub_gen_pi->add_option(
        "--adb-controller",
        args.adb_controller_filter,
        "Filter for ADB device auto-detection (matched against name/path/address).\n"
        "Only valid when the selected controller type is Adb.");
    sub_gen_pi->add_option("--resource", args.resource_name, "Resource name from interface.json (default: first)");
    sub_gen_pi->add_flag("--force", args.force, "Overwrite existing file if it already exists");
    sub_gen_pi->add_flag("--default-only", args.task_default_only, "Only include tasks with default_check=true");

    auto* sub_gen_task = sub_gen->add_subcommand("task", "Generate a sample task config file (TOML)");
    sub_gen_task->add_option("--output,-o", args.task_output_path, "Output file path (default: sample_task_config.toml)");
    sub_gen_task->add_flag("--force", args.force, "Overwrite existing file if it already exists");
    sub_gen_task->add_flag("--default-only", args.task_default_only, "Only include tasks with default_check=true");

    // ── help 子命令 ─────────────────────────────────────────────────────────
    auto* sub_help = app.add_subcommand(std::string(SubcommandName::Help), "Show help information (optionally for a subcommand)");
    sub_help->add_option("command", args.help_tokens, "Subcommand to show help for")->expected(0, -1);

    // 允许无子命令（交互模式）
    app.require_subcommand(0, 1);

    // 预生成 help 字符串供 `help <subcommand>` 子命令查表使用
    result.help_map[""] = app.help();
    result.help_map[std::string(SubcommandName::Run)] = sub_run->help();
    result.help_map[std::string(SubcommandName::List)] = sub_list->help();
    result.help_map[std::string(SubcommandName::GenerateConfig)] = sub_gen->help();
    result.help_map[std::string(SubcommandName::GenerateConfigPi)] = sub_gen_pi->help();
    result.help_map[std::string(SubcommandName::GenerateConfigTask)] = sub_gen_task->help();

    try {
        app.parse(argc, argv);
    }
    catch (const CLI::CallForVersion& e) {
        result.args.subcommand = Subcommand::ShowVersion;
        result.version_message = e.what();
        return result;
    }
    catch (const CLI::CallForHelp& e) {
        // -h/--help：让 CLI11 自动输出正确的子命令 help
        app.exit(e);
        result.args.subcommand = Subcommand::PrintTopHelp;
        result.already_printed = true;
        return result;
    }
    catch (const CLI::ParseError& e) {
        result.success = false;
        result.error_message = e.what();
        return result;
    }

    // 确定子命令
    if (sub_run->parsed()) {
        args.subcommand = Subcommand::Run;
    }
    else if (sub_list->parsed()) {
        args.subcommand = Subcommand::List;
    }
    else if (sub_gen_pi->parsed()) {
        args.subcommand = Subcommand::GenerateConfigPi;
    }
    else if (sub_gen_task->parsed()) {
        args.subcommand = Subcommand::GenerateConfigTask;
    }
    else if (sub_help->parsed()) {
        args.subcommand = Subcommand::HelpCommand;
    }
    // else: Subcommand::None → 交互模式

    return result;
}

void print_help(const ParseResult& result, const std::string& subcommand)
{
    auto it = result.help_map.find(subcommand);
    if (it != result.help_map.end()) {
        std::cout << it->second;
        return;
    }

    // 未知子命令名
    std::cout << "Unknown subcommand: '" << subcommand << "'\n"
              << "Available subcommands: " << SubcommandName::Run << ", " << SubcommandName::List << ", " << SubcommandName::GenerateConfig
              << ", " << SubcommandName::Help << "\n";
}

