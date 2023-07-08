#pragma once

#include "Base/NonCopyable.hpp"

#include <filesystem>
#include <set>

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
    const MAA_PIPELINE_RES_NS::TaskData& get_task_data(const std::string& task_name) const;

private:
    bool open_and_parse_file(const std::filesystem::path& path);
    bool load_template_images(const std::filesystem::path& path);

    bool parse_json(const json::value& input);
    bool parse_task(const std::string& name, const json::value& input);

    bool parse_recognition(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Type& out_type,
                           MAA_PIPELINE_RES_NS::Recognition::Params& out_param);
    bool parse_direct_hit_params(const json::value& input, MAA_VISION_NS::DirectHitParams& output);
    bool parse_templ_matching_params(const json::value& input, MAA_VISION_NS::TemplMatchingParams& output);
    bool parse_ocr_params(const json::value& input, MAA_VISION_NS::OcrParams& output);

    bool parse_roi(const json::value& input, std::vector<cv::Rect>& output);

    bool parse_action(const json::value& input, MAA_PIPELINE_RES_NS::Action::Type& out_type,
                      MAA_PIPELINE_RES_NS::Action::Params& out_param);
    bool parse_click(const json::value& input, MAA_PIPELINE_RES_NS::Action::ClickParams& output);
    bool parse_swipe(const json::value& input, MAA_PIPELINE_RES_NS::Action::SwipeParams& output);
    bool parse_wait_freezes_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::WaitFreezesParams& output);
    bool parse_app_info(const json::value& input, MAA_PIPELINE_RES_NS::Action::AppInfo& output);

    bool parse_rect(const json::value& input_rect, cv::Rect& output);
    bool parse_action_target(const json::value& input, const std::string& key,
                             MAA_PIPELINE_RES_NS::Action::Target& output_type,
                             MAA_PIPELINE_RES_NS::Action::TargetParam& output_param);

private:
    using TaskDataMap = std::map<std::string, MAA_PIPELINE_RES_NS::TaskData>;

    TaskDataMap raw_data_;
    mutable TaskDataMap processed_data_;

    TemplateConfig template_mgr_;
};

MAA_RES_NS_END
