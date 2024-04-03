#pragma once

#include <filesystem>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "PipelineTypes.h"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class PipelineResMgr : public NonCopyable
{
public:
    using TaskDataMap = std::unordered_map<std::string, TaskData>;

public:
    bool load(const std::filesystem::path& path, bool is_base);
    void clear();

    const TaskData& get_task_data(const std::string& task_name);

    const std::vector<std::filesystem::path>& get_paths() const { return paths_; }

    const TaskDataMap& get_task_data_map() const { return task_data_map_; }

    std::vector<std::string> get_task_list() const;

public:
    static bool parse_config(
        const json::value& input,
        TaskDataMap& output,
        std::set<std::string>& existing_keys,
        const TaskDataMap& default_value);
    static bool parse_task(
        const std::string& name,
        const json::value& input,
        TaskData& output,
        const TaskData& default_value);

    static bool parse_recognition(
        const json::value& input,
        Recognition::Type& out_type,
        Recognition::Param& out_param,
        const Recognition::Type& default_type,
        const Recognition::Param& default_param);
    // static bool parse_direct_hit_param(
    //     const json::value& input,
    //     MAA_VISION_NS::DirectHitParam& output,
    //     const MAA_VISION_NS::DirectHitParam& default_value);
    static bool parse_template_matcher_param(
        const json::value& input,
        MAA_VISION_NS::TemplateMatcherParam& output,
        const MAA_VISION_NS::TemplateMatcherParam& default_value);
    static bool parse_feature_matcher_param(
        const json::value& input,
        MAA_VISION_NS::FeatureMatcherParam& output,
        const MAA_VISION_NS::FeatureMatcherParam& default_value);
    static bool parse_ocrer_param(
        const json::value& input,
        MAA_VISION_NS::OCRerParam& output,
        const MAA_VISION_NS::OCRerParam& default_value);
    static bool parse_custom_recognition_param(
        const json::value& input,
        MAA_VISION_NS::CustomRecognizerParam& output,
        const MAA_VISION_NS::CustomRecognizerParam& default_value);
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

    static bool parse_roi(
        const json::value& input,
        std::vector<cv::Rect>& output,
        const std::vector<cv::Rect>& default_value);
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
        const Action::Type& default_type,
        const Action::Param& default_param);
    static bool parse_click(
        const json::value& input,
        Action::ClickParam& output,
        const Action::ClickParam& default_value);
    static bool parse_swipe(
        const json::value& input,
        Action::SwipeParam& output,
        const Action::SwipeParam& default_value);
    static bool parse_press_key(
        const json::value& input,
        Action::KeyParam& output,
        const Action::KeyParam& default_value);
    static bool parse_input_text(
        const json::value& input,
        Action::TextParam& output,
        const Action::TextParam& default_value);
    static bool parse_app_info(
        const json::value& input,
        Action::AppParam& output,
        const Action::AppParam& default_value);
    static bool parse_custom_action_param(
        const json::value& input,
        Action::CustomParam& output,
        const Action::CustomParam& default_value);

    static bool parse_wait_freezes_param(
        const json::value& input,
        const std::string& key,
        WaitFreezesParam& output,
        const WaitFreezesParam& default_value);

    static bool parse_rect(const json::value& input_rect, cv::Rect& output);
    static bool parse_action_target(
        const json::value& input,
        const std::string& key,
        Action::Target& output,
        const Action::Target& default_value);

private:
    bool load_all_json(const std::filesystem::path& path);
    bool open_and_parse_file(
        const std::filesystem::path& path,
        std::set<std::string>& existing_keys);
    bool check_all_next_list() const;
    bool check_next_list(const TaskData::NextList& next_list) const;

private:
    std::vector<std::filesystem::path> paths_;
    TaskDataMap task_data_map_;
};

MAA_RES_NS_END
