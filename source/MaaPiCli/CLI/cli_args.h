#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// 子命令名集中定义，避免 cli_args.cpp / main.cpp / print_help 各处散落字面量
namespace SubcommandName
{
inline constexpr std::string_view Run = "run";
inline constexpr std::string_view List = "list";
inline constexpr std::string_view GenerateConfig = "generate-config";
inline constexpr std::string_view GenerateConfigPi = "generate-config pi";
inline constexpr std::string_view GenerateConfigTask = "generate-config task";
inline constexpr std::string_view Help = "help";
} // namespace SubcommandName

enum class Subcommand
{
    None,               // 无子命令 → 交互模式
    Run,                // run [--task-config <path>] [--progress-level <0-2>]
    List,               // list
    GenerateConfigPi,   // generate-config pi [...]
    GenerateConfigTask, // generate-config task [...]
    PrintTopHelp,       // 由 -h/--help 触发：打印顶层 help
    HelpCommand,        // 由 `help [tokens...]` 子命令触发：打印指定子命令 help
    ShowVersion,        // -v/--version
};

struct CliArgs
{
    Subcommand subcommand = Subcommand::None;

    // run 子命令专有
    std::string task_config_path; // --task-config <path>
    int progress_level = 1;       // --progress-level <0-2>，0=静默 1=Task级 2=Task+Node级

    // generate-config pi 子命令专有
    std::string controller_name;       // --controller <name>
    std::string adb_controller_filter; // --adb-controller <pattern>，过滤 ADB 设备（name/path/address 含此字符串）
    std::string resource_name;         // --resource <name>，按 name 指定 resource（默认取第一个）

    // generate-config 共用
    bool force = false;             // --force：文件已存在时强制覆盖，否则退出
    bool task_default_only = false; // --default-only：只处理 default_check=true 的任务（pi/task 子命令共用）

    // generate-config task 专有
    std::string task_output_path; // --output <path>，默认 sample_task_config.toml

    // help 子命令专有
    std::vector<std::string> help_tokens; // help [word ...] → 拼接为 "generate-config pi" 等
};

struct ParseResult
{
    bool success = true;
    bool already_printed = false; // help/version 已由 CLI11 输出，main 无需再打印
    std::string error_message;
    std::string version_message;
    CliArgs args;
    // key: "" = top-level, "run"/"list"/"generate-config" = subcommand
    std::unordered_map<std::string, std::string> help_map;
};

ParseResult parse_args(int argc, char** argv);
void print_help(const ParseResult& result, const std::string& subcommand = {});
