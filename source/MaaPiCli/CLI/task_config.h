#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "ProjectInterface/Types.h"

// 任务配置文件解析器
// 支持的格式: TOML (.toml) / JSON (.json)
//
// task_config 描述"本次要运行哪些任务、以及临时覆盖的选项"，不包含控制器/设备/资源等
// 持久化配置由 maa_pi_config.json 负责提供。
//
// 示例 (TOML):
//   [[task]]
//   name = "TaskA"
//
//   [[task.option]]
//   name  = "difficulty"
//   value = "hard"
//
//   [[task]]
//   name = "TaskB"
//
//   # 可选：覆盖全局选项
//   [[global_option]]
//   name  = "language"
//   value = "en"

struct TaskConfig
{
    std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Task> task;
    std::vector<MAA_PROJECT_INTERFACE_NS::Configuration::Option> global_option;
};

struct TaskConfigParseResult
{
    bool success = true;
    std::string error_message;
    TaskConfig config;
};

enum class TaskConfigFormat
{
    Toml,
    Json,
};

// 根据扩展名自动选择解析器（.toml → TOML，其他 → JSON）
TaskConfigParseResult parse_task_config_file(const std::filesystem::path& path);

// 从字符串内容解析，需显式指定格式
TaskConfigParseResult parse_task_config_string(const std::string& content, TaskConfigFormat format);
