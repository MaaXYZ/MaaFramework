#pragma once

#include "Utils/NonCopyable.hpp"

#include <filesystem>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "PipelineTypes.h"
#include "TemplateResMgr.h"

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
    const TemplateResMgr& get_template_mgr() const { return template_mgr_; }

public:
    static bool parse_config(const json::value& input, TaskDataMap& output, const TaskDataMap& default_value);
    static bool parse_task(const std::string& name, const json::value& input, TaskData& output,
                           const TaskData& default_value);

    static bool parse_recognition(const json::value& input, Recognition::Type& out_type, Recognition::Param& out_param,
                                  const Recognition::Type& default_type, const Recognition::Param& default_param);
    // static bool parse_direct_hit_param(const json::value& input, MAA_VISION_NS::DirectHitParam& output,
    //                                    const MAA_VISION_NS::DirectHitParam& default_value);
    static bool parse_templ_matching_param(const json::value& input, MAA_VISION_NS::TemplMatchingParam& output,
                                           const MAA_VISION_NS::TemplMatchingParam& default_value);
    static bool parse_ocr_param(const json::value& input, MAA_VISION_NS::OcrParam& output,
                                const MAA_VISION_NS::OcrParam& default_value);
    static bool parse_custom_recognizer_param(const json::value& input, MAA_VISION_NS::CustomParam& output,
                                              const MAA_VISION_NS::CustomParam& default_value);
    static bool parse_classifier_param(const json::value& input, MAA_VISION_NS::ClassifierParam& output,
                                       const MAA_VISION_NS::ClassifierParam& default_value);
    static bool parse_detector_param(const json::value& input, MAA_VISION_NS::DetectorParam& output,
                                     const MAA_VISION_NS::DetectorParam& default_value);

    static bool parse_roi(const json::value& input, std::vector<cv::Rect>& output,
                          const std::vector<cv::Rect>& default_value);

    static bool parse_action(const json::value& input, Action::Type& out_type, Action::Param& out_param,
                             const Action::Type& default_type, const Action::Param& default_param);
    static bool parse_click(const json::value& input, Action::ClickParam& output,
                            const Action::ClickParam& default_value);
    static bool parse_swipe(const json::value& input, Action::SwipeParam& output,
                            const Action::SwipeParam& default_value);
    static bool parse_key_press(const json::value& input, Action::KeyParam& output,
                                const Action::KeyParam& default_value);
    static bool parse_app_info(const json::value& input, Action::AppParam& output,
                               const Action::AppParam& default_value);
    static bool parse_custom_action_param(const json::value& input, Action::CustomParam& output,
                                          const Action::CustomParam& default_value);

    static bool parse_wait_freezes_param(const json::value& input, const std::string& key, WaitFreezesParam& output,
                                         const WaitFreezesParam& default_value);

    static bool parse_rect(const json::value& input_rect, cv::Rect& output);
    static bool parse_action_target(const json::value& input, const std::string& key, Action::Target& output,
                                    const Action::Target& default_value);

private:
    bool load_all_json(const std::filesystem::path& path);
    bool open_and_parse_file(const std::filesystem::path& path);
    bool load_template_images(const std::filesystem::path& path);
    bool check_all_next_list() const;
    bool check_next_list(const TaskData::NextList& next_list) const;

private:
    std::vector<std::filesystem::path> paths_;
    TaskDataMap task_data_map_;
    TemplateResMgr template_mgr_;
};

MAA_RES_NS_END
