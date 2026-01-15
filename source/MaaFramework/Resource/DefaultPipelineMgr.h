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

    template <typename T>
    struct is_shared_ptr : std::false_type
    {
    };

    template <typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {
    };

    template <typename RecoParam>
    RecoParam get_recognition_param(Recognition::Type type) const
    {
        auto iter = recognition_param_.find(type);
        if (iter == recognition_param_.end()) {
            if constexpr (is_shared_ptr<RecoParam>::value) {
                return std::make_shared<typename RecoParam::element_type>();
            }
            else {
                return {};
            }
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
