#pragma once

#include "Base/NonCopyable.hpp"

#include <filesystem>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Common/MaaConf.h"
#include "PipelineTypes.h"
#include "TemplateConfig.h"

MAA_RES_NS_BEGIN

class PipelineConfig : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path, bool is_base);
    void clear();

public:
    using TaskDataMap = std::unordered_map<std::string, MAA_PIPELINE_RES_NS::TaskData>;

public:
    const MAA_PIPELINE_RES_NS::TaskData& get_task_data(const std::string& task_name);
    const TaskDataMap& get_task_data_map() const { return task_data_map_; }

public:
    static bool parse_json(const json::value& input, TaskDataMap& output, const TaskDataMap& default_value);
    static bool parse_task(const std::string& name, const json::value& input, MAA_PIPELINE_RES_NS::TaskData& output,
                           const MAA_PIPELINE_RES_NS::TaskData& default_value);

    static bool parse_recognition(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Type& out_type,
                                  MAA_PIPELINE_RES_NS::Recognition::Params& out_param,
                                  const MAA_PIPELINE_RES_NS::Recognition::Type& default_type,
                                  const MAA_PIPELINE_RES_NS::Recognition::Params& default_param);
    static bool parse_direct_hit_params(const json::value& input, MAA_VISION_NS::DirectHitParams& output,
                                        const MAA_VISION_NS::DirectHitParams& default_value);
    static bool parse_templ_matching_params(const json::value& input, MAA_VISION_NS::TemplMatchingParams& output,
                                            const MAA_VISION_NS::TemplMatchingParams& default_value);
    static bool parse_ocr_params(const json::value& input, MAA_VISION_NS::OcrParams& output,
                                 const MAA_VISION_NS::OcrParams& default_value);

    static bool parse_roi(const json::value& input, std::vector<cv::Rect>& output,
                          const std::vector<cv::Rect>& default_value);

    static bool parse_action(const json::value& input, MAA_PIPELINE_RES_NS::Action::Type& out_type,
                             MAA_PIPELINE_RES_NS::Action::Params& out_param,
                             const MAA_PIPELINE_RES_NS::Action::Type& default_type,
                             const MAA_PIPELINE_RES_NS::Action::Params& default_param);
    static bool parse_click(const json::value& input, MAA_PIPELINE_RES_NS::Action::ClickParams& output,
                            const MAA_PIPELINE_RES_NS::Action::ClickParams& default_value);
    static bool parse_swipe(const json::value& input, MAA_PIPELINE_RES_NS::Action::SwipeParams& output,
                            const MAA_PIPELINE_RES_NS::Action::SwipeParams& default_value);
    static bool parse_key_press(const json::value& input, MAA_PIPELINE_RES_NS::Action::KeyParams& output,
                                const MAA_PIPELINE_RES_NS::Action::KeyParams& default_value);
    static bool parse_app_info(const json::value& input, MAA_PIPELINE_RES_NS::Action::AppInfo& output,
                               const MAA_PIPELINE_RES_NS::Action::AppInfo& default_value);
    static bool parse_custom_task_params(const json::value& input,
                                         MAA_PIPELINE_RES_NS::Action::CustomTaskParams& output,
                                         const MAA_PIPELINE_RES_NS::Action::CustomTaskParams& default_value);

    static bool parse_wait_freezes_params(const json::value& input, const std::string& key,
                                          MAA_PIPELINE_RES_NS::WaitFreezesParams& output,
                                          const MAA_PIPELINE_RES_NS::WaitFreezesParams& default_value);

    static bool parse_rect(const json::value& input_rect, cv::Rect& output);
    static bool parse_action_target(const json::value& input, const std::string& key,
                                    MAA_PIPELINE_RES_NS::Action::Target& output_type,
                                    MAA_PIPELINE_RES_NS::Action::TargetParam& output_param,
                                    const MAA_PIPELINE_RES_NS::Action::Target& default_type,
                                    const MAA_PIPELINE_RES_NS::Action::TargetParam& default_param);

private:
    bool load_all_json(const std::filesystem::path& path);
    bool open_and_parse_file(const std::filesystem::path& path);
    bool load_template_images(const std::filesystem::path& path);
    bool check_all_next_list() const;
    bool check_next_list(const MAA_PIPELINE_RES_NS::TaskData::NextList& next_list) const;

private:
    TaskDataMap task_data_map_;
    TemplateConfig template_mgr_;
};

MAA_RES_NS_END
