#pragma once

#include <filesystem>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "DefaultPipelineMgr.h"
#include "PipelineTypes.h"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class PipelineResMgr : public NonCopyable
{
public:
    using PipelineDataMap = std::unordered_map<std::string, PipelineData>;

public:
    bool load(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr);
    void clear();

    const std::vector<std::filesystem::path>& get_paths() const { return paths_; }

    const PipelineDataMap& get_pipeline_data_map() const { return pipeline_data_map_; }

    PipelineDataMap& get_pipeline_data_map() { return pipeline_data_map_; }

    std::vector<std::string> get_node_list() const;

public:
    bool parse_and_override(const json::object& input, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mg);

public:
    static bool parse_node(
        const std::string& name,
        const json::value& input,
        PipelineData& output,
        const PipelineData& parent_values,
        const DefaultPipelineMgr& default_mgr);

    static bool parse_recognition(
        const json::value& input,
        Recognition::Type& out_type,
        Recognition::Param& out_param,
        const Recognition::Type& parent_type,
        const Recognition::Param& parent_param,
        const DefaultPipelineMgr& default_mgr);

    static bool parse_template_matcher_param(
        const json::value& input,
        MAA_VISION_NS::TemplateMatcherParam& output,
        const MAA_VISION_NS::TemplateMatcherParam& default_value);
    static bool parse_feature_matcher_param(
        const json::value& input,
        MAA_VISION_NS::FeatureMatcherParam& output,
        const MAA_VISION_NS::FeatureMatcherParam& default_value);
    static bool
        parse_ocrer_param(const json::value& input, MAA_VISION_NS::OCRerParam& output, const MAA_VISION_NS::OCRerParam& default_value);
    static bool parse_custom_recognition_param(
        const json::value& input,
        MAA_VISION_NS::CustomRecognitionParam& output,
        const MAA_VISION_NS::CustomRecognitionParam& default_value);
    static bool parse_nn_classifier_param(
        const json::value& input,
        MAA_VISION_NS::NeuralNetworkClassifierParam& output,
        const MAA_VISION_NS::NeuralNetworkClassifierParam& default_value);
    static bool parse_nn_detector_param(
        const json::value& input,
        MAA_VISION_NS::NeuralNetworkDetectorParam& output,
        const MAA_VISION_NS::NeuralNetworkDetectorParam& default_value);
    static bool parse_color_matcher_param(
        const json::value& input,
        MAA_VISION_NS::ColorMatcherParam& output,
        const MAA_VISION_NS::ColorMatcherParam& default_value);

    static bool parse_roi_target(const json::value& input, MAA_VISION_NS::Target& output, const MAA_VISION_NS::Target& default_value);

    static bool parse_order_of_result(
        const json::value& input,
        MAA_VISION_NS::ResultOrderBy& output,
        int& output_index,
        const MAA_VISION_NS::ResultOrderBy& default_value,
        int default_index,
        const std::unordered_set<MAA_VISION_NS::ResultOrderBy>& valid_values);

    static bool parse_action(
        const json::value& input,
        Action::Type& out_type,
        Action::Param& out_param,
        const Action::Type& parent_type,
        const Action::Param& parent_param,
        const DefaultPipelineMgr& default_mgr);
    static bool parse_click(const json::value& input, Action::ClickParam& output, const Action::ClickParam& default_value);
    static bool parse_swipe(const json::value& input, Action::SwipeParam& output, const Action::SwipeParam& default_value);
    static bool parse_multi_swipe(
        const json::value& input,
        Action::MultiSwipeParam& output,
        const Action::MultiSwipeParam& default_mluti,
        const Action::SwipeParam& default_single);
    static bool parse_press_key(const json::value& input, Action::KeyParam& output, const Action::KeyParam& default_value);
    static bool parse_input_text(const json::value& input, Action::TextParam& output, const Action::TextParam& default_value);
    static bool parse_app_info(const json::value& input, Action::AppParam& output, const Action::AppParam& default_value);
    static bool parse_command_param(const json::value& input, Action::CommandParam& output, const Action::CommandParam& default_value);
    static bool parse_custom_action_param(const json::value& input, Action::CustomParam& output, const Action::CustomParam& default_value);

    static bool parse_wait_freezes_param(
        const json::value& input,
        const std::string& key,
        WaitFreezesParam& output,
        const WaitFreezesParam& default_value);

    static bool parse_rect(const json::value& input_rect, cv::Rect& output);
    static bool parse_target_variant(const json::value& input_target, Action::Target& output);
    static bool parse_target_offset(const json::value& input_target, Action::Target& output);
    static bool
        parse_action_target(const json::value& input, const std::string& key, Action::Target& output, const Action::Target& default_value);

    static bool check_all_validity(const PipelineDataMap& data_map);
    static bool check_all_next_list(const PipelineDataMap& data_map);
    static bool check_all_regex(const PipelineDataMap& data_map);

private:
    bool load_all_json(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr);
    bool
        open_and_parse_file(const std::filesystem::path& path, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mgr);

    static bool check_next_list(const PipelineData::NextList& next_list, const PipelineDataMap& data_map);

private:
    std::vector<std::filesystem::path> paths_;
    PipelineDataMap pipeline_data_map_;
};

MAA_RES_NS_END
