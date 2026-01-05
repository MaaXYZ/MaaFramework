#pragma once

#include <filesystem>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaUtils/NonCopyable.hpp"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class DefaultPipelineMgr : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path);
    void clear();

public:
    const PipelineData& get_pipeline() const { return pipeline_param_; }

    template <typename RecoParam>
    RecoParam get_recognition_param(Recognition::Type type) const
    {
        auto iter = recognition_param_.find(type);
        if (iter == recognition_param_.end()) {
            return {};
        }
        return std::get<RecoParam>(iter->second);
    }

    template <typename ActParam>
    ActParam get_action_param(Action::Type type) const
    {
        auto iter = action_param_.find(type);
        if (iter == action_param_.end()) {
            return {};
        }
        return std::get<ActParam>(iter->second);
    }

private:
    bool parse_pipeline(const json::value& input);
    bool parse_recognition(const json::value& input);
    bool parse_action(const json::value& input);

    void add_recognition_param(Recognition::Type, Recognition::Param param);
    void add_action_param(Action::Type, Action::Param param);

private:
    PipelineData pipeline_param_;
    std::unordered_map<Recognition::Type, Recognition::Param> recognition_param_;
    std::unordered_map<Action::Type, Action::Param> action_param_;
};

MAA_RES_NS_END
