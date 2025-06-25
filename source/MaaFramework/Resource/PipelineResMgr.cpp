#include "PipelineResMgr.h"

#include <ranges>

#include "PipelineChecker.h"
#include "PipelineParser.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_RES_NS_BEGIN

bool PipelineResMgr::load(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr)
{
    LogFunc << VAR(path);

    if (!load_all_json(path, default_mgr)) {
        LogError << "load_all_json failed" << VAR(path);
        return false;
    }

    if (!PipelineChecker::check_all_validity(pipeline_data_map_)) {
        LogError << "check_all_validity failed" << VAR(path);
        return false;
    }

    paths_.emplace_back(path);

    return true;
}

void PipelineResMgr::clear()
{
    LogFunc;

    pipeline_data_map_.clear();
    paths_.clear();
}

bool PipelineResMgr::load_all_json(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr)
{
    if (!std::filesystem::is_directory(path)) {
        LogError << "path is not directory" << VAR(path);
        return false;
    }

    bool valid = false;

    std::set<std::string> existing_keys;
    for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        auto& entry_path = entry.path();
        if (entry.is_directory()) {
            LogDebug << "entry is directory" << VAR(entry_path);
            continue;
        }
        if (!entry.is_regular_file()) {
            LogWarn << "entry is not regular file, skip" << VAR(entry_path);
            continue;
        }
        auto relative = std::filesystem::relative(entry_path, path);
        for (const auto& part : relative) {
            if (part.string().starts_with('.')) {
                LogWarn << "entry starts with . skip" << VAR(entry_path) << VAR(part);
                continue;
            }
        }

        auto ext = path_to_utf8_string(entry_path.extension());
        tolowers_(ext);
        if (ext != ".json" && ext != ".jsonc") {
            LogWarn << "entry is not *.json or *.jsonc, skip" << VAR(entry_path) << VAR(ext);
            continue;
        }

        bool parsed = open_and_parse_file(entry_path, existing_keys, default_mgr);
        if (!parsed) {
            LogError << "open_and_parse_file failed" << VAR(entry_path);
            return false;
        }

        valid = true;
    }

    return valid;
}

bool PipelineResMgr::open_and_parse_file(
    const std::filesystem::path& path,
    std::set<std::string>& existing_keys,
    const DefaultPipelineMgr& default_mgr)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path, true, true);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }
    const auto& json = *json_opt;

    if (!json.is_object()) {
        LogError << "json is not object";
        return false;
    }

    if (!parse_and_override(json.as_object(), existing_keys, default_mgr)) {
        LogError << "parse_config failed" << VAR(path) << VAR(json);
        return false;
    }

    return true;
}

std::vector<std::string> PipelineResMgr::get_node_list() const
{
    auto k = pipeline_data_map_ | std::views::keys;
    return std::vector(k.begin(), k.end());
}

bool PipelineResMgr::parse_and_override(
    const json::object& input,
    std::set<std::string>& existing_keys,
    const DefaultPipelineMgr& default_mgr)
{
    for (const auto& [key, value] : input) {
        if (key.empty()) {
            LogError << "key is empty" << VAR(key);
            return false;
        }
        if (key.starts_with('$')) {
            LogInfo << "key starts with '$', skip" << VAR(key);
            continue;
        }
        if (existing_keys.contains(key)) {
            LogError << "key already exists" << VAR(key);
            return false;
        }
        if (!value.is_object()) {
            LogError << "value is not object" << VAR(key) << VAR(value);
            return false;
        }

        PipelineData result;
        const auto& default_result = pipeline_data_map_.contains(key) ? pipeline_data_map_.at(key) : default_mgr.get_pipeline();
        bool ret = PipelineParser::parse_node(key, value, result, default_result, default_mgr);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }

        existing_keys.emplace(key);
        pipeline_data_map_.insert_or_assign(key, std::move(result));
    }

    return true;
}

MAA_RES_NS_END
