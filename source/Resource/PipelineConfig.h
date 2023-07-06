#pragma once

#include "Base/NonCopyable.hpp"

#include <filesystem>

#include <meojson/json.hpp>

#include "Common/MaaConf.h"
#include "PipelineTypes.h"

MAA_RES_NS_BEGIN

class PipelineConfig : public NonCopyable
{
public:
    bool load(const std::filesystem::path& path, bool is_base);
    void clear();
    const auto& get_image_requirement() const { return image_requirement_; }

public:
    const MAA_PIPELINE_RES_NS::TaskData& get_data(const std::string& task_name) const
    {
        return raw_data_.at(task_name);
    }

private:
    bool open_and_parse_file(const std::filesystem::path& path);
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

    bool parse_rect(const json::value& input_rect, cv::Rect& output);
    bool parse_action_target(const json::value& input, const std::string& key,
                             MAA_PIPELINE_RES_NS::Action::Target& output_type,
                             MAA_PIPELINE_RES_NS::Action::TargetParam& output_param);

private:
    std::unordered_map<std::string, MAA_PIPELINE_RES_NS::TaskData> raw_data_;
    std::map<std::string, std::filesystem::path> image_requirement_;
};

MAA_RES_NS_END
