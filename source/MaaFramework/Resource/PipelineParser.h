#pragma once

#include <filesystem>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "DefaultPipelineMgr.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineParser
{
public:
    PipelineParser() = delete;

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

    static bool parse_action(
        const json::value& input,
        Action::Type& out_type,
        Action::Param& out_param,
        const Action::Type& parent_type,
        const Action::Param& parent_param,
        const DefaultPipelineMgr& default_mgr);

    static bool parse_next(const json::value& input, std::vector<NodeAttr>& output);

private:
    static bool parse_direct_hit_param(
        const json::value& input,
        MAA_VISION_NS::DirectHitParam& output,
        const MAA_VISION_NS::DirectHitParam& default_value);
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
    static bool
        parse_and_param(const json::value& input, std::shared_ptr<Recognition::AndParam>& output, const DefaultPipelineMgr& default_mgr);
    static bool
        parse_or_param(const json::value& input, std::shared_ptr<Recognition::OrParam>& output, const DefaultPipelineMgr& default_mgr);
    static bool parse_sub_recognition(const json::value& input, Recognition::SubRecognition& output, const DefaultPipelineMgr& default_mgr);

    static bool parse_roi_target(const json::value& input, MAA_VISION_NS::Target& output, const MAA_VISION_NS::Target& default_value);

    static bool parse_order_of_result(
        const json::value& input,
        MAA_VISION_NS::ResultOrderBy& output,
        int& output_index,
        const MAA_VISION_NS::ResultOrderBy& default_value,
        int default_index,
        const std::unordered_set<MAA_VISION_NS::ResultOrderBy>& valid_values);

    static bool parse_click(const json::value& input, Action::ClickParam& output, const Action::ClickParam& default_value);
    static bool parse_long_press(const json::value& input, Action::LongPressParam& output, const Action::LongPressParam& default_value);
    static bool parse_swipe(const json::value& input, Action::SwipeParam& output, const Action::SwipeParam& default_value);
    static bool parse_multi_swipe(
        const json::value& input,
        Action::MultiSwipeParam& output,
        const Action::MultiSwipeParam& default_mluti,
        const Action::SwipeParam& default_single);
    static bool parse_key_param(const json::value& input, Action::KeyParam& output, const Action::KeyParam& default_value);
    static bool parse_touch(const json::value& input, Action::TouchParam& output, const Action::TouchParam& default_value);
    static bool parse_touch_up(const json::value& input, Action::TouchUpParam& output, const Action::TouchUpParam& default_value);
    static bool parse_click_key(const json::value& input, Action::ClickKeyParam& output, const Action::ClickKeyParam& default_value);
    static bool
        parse_long_press_key(const json::value& input, Action::LongPressKeyParam& output, const Action::LongPressKeyParam& default_value);
    static bool parse_input_text(const json::value& input, Action::InputTextParam& output, const Action::InputTextParam& default_value);
    static bool parse_app_info(const json::value& input, Action::AppParam& output, const Action::AppParam& default_value);
    static bool parse_scroll(const json::value& input, Action::ScrollParam& output, const Action::ScrollParam& default_value);
    static bool parse_shell(const json::value& input, Action::ShellParam& output, const Action::ShellParam& default_value);
    static bool parse_command_param(const json::value& input, Action::CommandParam& output, const Action::CommandParam& default_value);
    static bool parse_custom_action_param(const json::value& input, Action::CustomParam& output, const Action::CustomParam& default_value);

    static bool parse_wait_freezes_param(
        const json::value& input,
        const std::string& key,
        WaitFreezesParam& output,
        const WaitFreezesParam& default_value);

    static bool parse_rect(const json::value& input_rect, cv::Rect& output);
    template <typename TargetType>
    static bool parse_target_variant(const json::value& input_target, TargetType& output);
    static bool
        parse_action_target(const json::value& input, const std::string& key, Action::Target& output, const Action::Target& default_value);
    static bool parse_action_target_obj_or_list(
        const json::value& input,
        const std::string& key,
        std::vector<Action::TargetObj>& output,
        const std::vector<Action::TargetObj>& default_value);
    static bool parse_action_target_offset_or_list(
        const json::value& input,
        const std::string& key,
        std::vector<cv::Rect>& output,
        const std::vector<cv::Rect>& default_value);

    static bool parse_next(
        const json::value& input,
        const std::string& key,
        std::vector<NodeAttr>& output,
        const std::vector<NodeAttr>& default_next);
    static bool parse_node_in_next(const json::value& input, NodeAttr& output);
    static bool parse_node_string_in_next(const std::string& raw, NodeAttr& output);
};

MAA_RES_NS_END
