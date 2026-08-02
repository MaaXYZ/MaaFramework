#include "task_config.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <meojson/json.hpp>
#include <tomlplusplus/toml.hpp>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

namespace
{

// 将 toml++ 节点递归转换为 meojson::value
json::value toml_to_json(const toml::node& node)
{
    if (auto* tbl = node.as_table()) {
        json::object obj;
        for (auto&& [k, v] : *tbl) {
            obj.emplace(std::string(k.str()), toml_to_json(v));
        }
        return obj;
    }
    if (auto* arr = node.as_array()) {
        json::array out;
        for (auto&& item : *arr) {
            out.emplace_back(toml_to_json(item));
        }
        return out;
    }
    if (auto* str = node.as_string()) {
        return std::string(str->get());
    }
    if (auto* i = node.as_integer()) {
        return i->get();
    }
    if (auto* f = node.as_floating_point()) {
        return f->get();
    }
    if (auto* b = node.as_boolean()) {
        return b->get();
    }
    if (auto* dt = node.as_date_time()) {
        std::ostringstream oss;
        oss << *dt;
        return oss.str();
    }
    if (auto* d = node.as_date()) {
        std::ostringstream oss;
        oss << *d;
        return oss.str();
    }
    if (auto* t = node.as_time()) {
        std::ostringstream oss;
        oss << *t;
        return oss.str();
    }
    return json::value();
}

TaskConfigFormat detect_format_from_path(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == ".toml") {
        return TaskConfigFormat::Toml;
    }
    return TaskConfigFormat::Json;
}

// 从 meojson 对象中解析 TaskConfig（只取 task / global_option）
TaskConfigParseResult parse_from_json_value(const json::value& jv)
{
    using namespace MAA_PROJECT_INTERFACE_NS;

    TaskConfigParseResult result;

    if (!jv.is_object()) {
        result.success = false;
        result.error_message = "Task config must be a JSON object / TOML table";
        return result;
    }

    // task 列表（必须存在且非空）
    if (auto tasks_val = jv.find("task")) {
        if (!tasks_val->is_array()) {
            result.success = false;
            result.error_message = "'task' must be an array";
            return result;
        }
        for (const auto& tv : tasks_val->as_array()) {
            Configuration::Task t;
            std::string err;
            if (!t.check_json(tv, err)) {
                result.success = false;
                result.error_message = "Invalid task entry, error key: '" + err + "'";
                return result;
            }
            result.config.task.emplace_back(tv.as<Configuration::Task>());
        }
    }

    if (result.config.task.empty()) {
        result.success = false;
        result.error_message = "Task config must contain at least one [[task]] entry";
        return result;
    }

    // global_option（可选）
    if (auto opts_val = jv.find("global_option")) {
        if (!opts_val->is_array()) {
            result.success = false;
            result.error_message = "'global_option' must be an array";
            return result;
        }
        for (const auto& ov : opts_val->as_array()) {
            Configuration::Option o;
            std::string err;
            if (!o.check_json(ov, err)) {
                result.success = false;
                result.error_message = "Invalid global_option entry, error key: '" + err + "'";
                return result;
            }
            result.config.global_option.emplace_back(ov.as<Configuration::Option>());
        }
    }

    return result;
}

} // namespace

TaskConfigParseResult parse_task_config_string(const std::string& content, TaskConfigFormat format)
{
    LogFunc;

    TaskConfigParseResult result;

    if (format == TaskConfigFormat::Json) {
        auto parsed = json::parse(content);
        if (!parsed) {
            result.success = false;
            result.error_message = "Failed to parse JSON content";
            return result;
        }
        return parse_from_json_value(*parsed);
    }

    // TOML
    try {
        auto tbl = toml::parse(content);
        json::object root_obj;
        for (auto&& [k, v] : tbl) {
            root_obj.emplace(std::string(k.str()), toml_to_json(v));
        }
        return parse_from_json_value(json::value(std::move(root_obj)));
    }
    catch (const toml::parse_error& e) {
        std::ostringstream oss;
        oss << "TOML parse error: " << e.description() << " at " << e.source().begin;
        result.success = false;
        result.error_message = oss.str();
        return result;
    }
}

TaskConfigParseResult parse_task_config_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    TaskConfigParseResult result;

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        result.success = false;
        result.error_message = "Failed to open task config file: " + MAA_NS::path_to_utf8_string(path);
        return result;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    auto content = buffer.str();

    // 剥 UTF-8 BOM (EF BB BF)：Windows 记事本默认会写 BOM，而 meojson 不容忍前导非空白，
    // toml++ 虽然能容忍但不同版本行为不一，统一在这里处理一次更稳。
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF && static_cast<unsigned char>(content[1]) == 0xBB
        && static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    auto format = detect_format_from_path(path);
    return parse_task_config_string(content, format);
}
