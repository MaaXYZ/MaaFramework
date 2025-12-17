#pragma once

#include <filesystem>
#include <set>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "DefaultPipelineMgr.h"
#include "MaaUtils/NonCopyable.hpp"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineResMgr : public NonCopyable
{
public:
    inline static constexpr std::string_view kFilePrefix_Ignore = ".";

public:
    bool load(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr);
    bool load_file(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr);
    void clear();

    const std::vector<std::filesystem::path>& get_paths() const { return paths_; }

    const PipelineDataMap& get_pipeline_data_map() const { return pipeline_data_map_; }

    PipelineDataMap& get_pipeline_data_map() { return pipeline_data_map_; }

    std::vector<std::string> get_node_list() const;

public:
    bool parse_and_override(const json::value& input, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mgr);

private:
    bool load_all_json(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr);
    bool
        open_and_parse_file(const std::filesystem::path& path, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mgr);
    bool parse_and_override_once(const json::object& input, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mgr);

private:
    std::vector<std::filesystem::path> paths_;
    PipelineDataMap pipeline_data_map_;
};

MAA_RES_NS_END
