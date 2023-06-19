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

private:
    bool open_and_parse_file(const std::filesystem::path& path);
    bool parse_json(const json::value& input);
    bool parse_task(const std::string& name, const json::value& input);

    bool parse_recognition(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Type& out_type,
                           MAA_PIPELINE_RES_NS::Recognition::Params& out_param);
    bool parse_templ_matching_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);
    bool parse_ocr_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);
    bool parse_freezes_wait_params(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Params& output);

    bool parse_roi(const json::value& input, std::vector<cv::Rect>& output);

    bool parse_action(const json::value& input, MAA_PIPELINE_RES_NS::Action::Type& out_type,
                      MAA_PIPELINE_RES_NS::Action::Params& out_param);
    bool parse_click_self_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output);
    bool parse_click_region_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output);
    bool parse_swipe_self_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output);
    bool parse_swipe_region_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output);

    bool parse_rect(const json::value& input_rect, cv::Rect& output);

    std::unordered_map<std::string, MAA_PIPELINE_RES_NS::Data> data_;
};

MAA_RES_NS_END
