#include "PipelineConfig.h"

#include "Utils/Logger.hpp"

MAA_RES_NS_BEGIN

using namespace MAA_PIPELINE_RES_NS;

bool PipelineConfig::load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        data_.clear();
    }

    if (!std::filesystem::exists(path)) {
        LogError << "path not exists";
        return false;
    }
    else if (std::filesystem::is_directory(path)) {
        for (auto& entry : std::filesystem::directory_iterator(path)) {
            bool ret = open_and_parse_file(entry.path());
            if (!ret) {
                LogError << "open_and_parse_file failed" << VAR(entry.path());
                return false;
            }
        }
    }
    else if (std::filesystem::is_regular_file(path)) {
        bool ret = open_and_parse_file(path);
        if (!ret) {
            LogError << "open_and_parse_file failed" << VAR(path);
            return false;
        }
    }
    else {
        LogError << "path is not directory or regular file";
        return false;
    }

    return true;
}

bool PipelineConfig::open_and_parse_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }

    return parse_json(*json_opt);
}

bool PipelineConfig::parse_json(const json::value& input)
{
    if (!input.is_object()) {
        LogError << "json is not object";
        return false;
    }

    for (const auto& [key, value] : input.as_object()) {
        bool ret = parse_task(key, value);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }
    }
    return true;
}

template <typename OutT>
bool get_and_check(const json::value& input, const std::string& key, OutT& output, const OutT& default_val)
{
    auto opt = input.find<OutT>(key);
    if (!opt) {
        if (input.exists(key)) {
            LogError << "type error" << VAR(key) << VAR(input);
            return false;
        }
        output = default_val;
    }
    else {
        output = *opt;
    }
    return true;
}

bool PipelineConfig::parse_task(const std::string& name, const json::value& input)
{
    LogFunc << VAR(name);

    Data data;
    data.name = name;

    if (!parse_recognition(input, data.rec_type, data.rec_params)) {
        LogError << "failed to parse_recognition" << VAR(input);
        return false;
    }

    if (!parse_roi(input, data.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!get_and_check(input, "cache", data.cache, false)) {
        LogError << "failed to get_and_check cache" << VAR(input);
        return false;
    }

    if (!parse_action(input, data.action_type, data.action_params)) {
        LogError << "failed to parse_action" << VAR(input);
        return false;
    }
    if (!parse_next(input, "next", data.next)) {
        LogError << "failed to parse_next next" << VAR(input);
        return false;
    }

    json::value v(0U);

    if (!get_and_check(input, "timeout", data.timeout, 10 * 1000U)) {
        LogError << "failed to get_and_check timeout" << VAR(input);
        return false;
    }

    if (!parse_next(input, "timeout_next", data.next)) {
        LogError << "failed to parse_next timeout_next" << VAR(input);
        return false;
    }

    if (!get_and_check(input, "run_times", data.run_times, uint(UINT_MAX))) {
        LogError << "failed to get_and_check run_times" << VAR(input);
        return false;
    }

    if (!parse_next(input, "runout_next", data.runout_next)) {
        LogError << "failed to parse_next runout_next" << VAR(input);
        return false;
    }

    if (!get_and_check(input, "pre_delay", data.pre_delay, 0U)) {
        LogError << "failed to get_and_check pre_delay" << VAR(input);
        return false;
    }

    if (!get_and_check(input, "post_delay", data.post_delay, 0U)) {
        LogError << "failed to get_and_check post_delay" << VAR(input);
        return false;
    }

    if (!get_and_check(input, "notify", data.notify, false)) {
        LogError << "failed to get_and_check notify" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_recognition(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Type& out_type,
                                       MAA_PIPELINE_RES_NS::Recognition::Params& out_param)
{
    std::string rec_type_name;
    if (!get_and_check(input, "recognition", rec_type_name, std::string("DirectHit"))) {
        LogError << "failed to get_and_check recognition" << VAR(input);
        return false;
    }

    static const std::unordered_map<std::string, Recognition::Type> kRecTypeMap = {
        { "DirectHit", Recognition::Type::DirectHit },     { "TemplateMatch", Recognition::Type::TemplateMatch },
        { "OcrPipeline", Recognition::Type::OcrPipeline }, { "OcrRec", Recognition::Type::OcrRec },
        { "FreezesWait", Recognition::Type::FreezesWait },
    };
    auto rec_type_iter = kRecTypeMap.find(rec_type_name);
    if (rec_type_iter == kRecTypeMap.end()) {
        LogError << "rec type not found" << VAR(rec_type_name);
        return false;
    }
    out_type = rec_type_iter->second;

    switch (out_type) {
    case Recognition::Type::DirectHit:
        return true;
    case Recognition::Type::TemplateMatch:
        return parse_templ_matching_params(input, out_param);
    case Recognition::Type::OcrPipeline:
    case Recognition::Type::OcrRec:
        return parse_ocr_params(input, out_param);
        break;
    case Recognition::Type::FreezesWait:
        return parse_freezes_wait_params(input, out_param);
        break;
    default:
        return false;
    }

    return false;
}

bool PipelineConfig::parse_templ_matching_params(const json::value& input, Recognition::Params& output)
{
    return false;
}

bool PipelineConfig::parse_ocr_params(const json::value& input, Recognition::Params& output)
{
    return false;
}

bool PipelineConfig::parse_freezes_wait_params(const json::value& input, Recognition::Params& output)
{
    return false;
}

bool PipelineConfig::parse_roi(const json::value& input, std::vector<cv::Rect>& roi)
{
    roi.clear();

    auto roi_opt = input.find<json::array>("roi");
    if (!roi_opt) {
        return true;
    }
    auto& roi_array = *roi_opt;
    if (roi_array.empty()) {
        LogError << "roi array is empty" << VAR(input);
        return false;
    }

    auto parse_single_roi = [&](const json::array& int_array) {
        if (int_array.size() != 4) {
            LogError << "roi size != 4" << VAR(int_array.size());
            return false;
        }
        std::vector<int> roi_vec;
        for (const auto& r : int_array) {
            if (!r.is_number()) {
                LogError << "type error" << VAR(r) << "is not integer";
                return false;
            }
            roi_vec.emplace_back(r.as_integer());
        }
        roi.emplace_back(roi_vec[0], roi_vec[1], roi_vec[2], roi_vec[3]);
        return true;
    };

    auto type = roi_array.begin()->type();
    switch (type) {
    case json::value::value_type::number:
        return parse_single_roi(roi_array);
    case json::value::value_type::array: {
        bool parsed = true;
        for (const auto& r : roi_array) {
            if (!r.is_array()) {
                LogError << "type error" << VAR(r) << "is not json::array";
                return false;
            }
            parsed &= parse_single_roi(r.as_array());
        }
        return parsed;
    } break;
    default:
        LogError << "error roi type" << VAR(input);
        return false;
    }

    return false;
}

bool PipelineConfig::parse_action(const json::value& input, MAA_PIPELINE_RES_NS::Action::Type& out_type,
                                  MAA_PIPELINE_RES_NS::Action::Params& out_param)
{
    std::string act_type_name;
    if (!get_and_check(input, "action", act_type_name, std::string("DoNothing"))) {
        LogError << "failed to get_and_check action" << VAR(input);
        return false;
    }

    static const std::unordered_map<std::string, Action::Type> kActTypeMap = {
        { "DoNothing", Action::Type::DoNothing },     { "ClickSelf", Action::Type::ClickSelf },
        { "ClickRegion", Action::Type::ClickRegion }, { "SwipeSelf", Action::Type::SwipeSelf },
        { "SwipeRegion", Action::Type::SwipeRegion },
    };
    auto act_type_iter = kActTypeMap.find(act_type_name);
    if (act_type_iter == kActTypeMap.cend()) {
        LogError << "act type not found" << VAR(act_type_name);
        return false;
    }
    out_type = act_type_iter->second;

    switch (out_type) {
    case Action::Type::DoNothing:
        return true;
    case Action::Type::ClickSelf:
        return parse_click_self_params(input, out_param);
    case Action::Type::ClickRegion:
        return parse_click_region_params(input, out_param);
    case Action::Type::SwipeSelf:
        return parse_swipe_self_params(input, out_param);
    case Action::Type::SwipeRegion:
        return parse_swipe_region_params(input, out_param);
    default:
        return false;
    }

    return false;
}

bool PipelineConfig::parse_click_self_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output)
{
    return false;
}

bool PipelineConfig::parse_click_region_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output)
{
    return false;
}

bool PipelineConfig::parse_swipe_self_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output)
{
    return false;
}

bool PipelineConfig::parse_swipe_region_params(const json::value& input, MAA_PIPELINE_RES_NS::Action::Params& output)
{
    return false;
}

bool PipelineConfig::parse_next(const json::value& input, const std::string& key, std::vector<std::string>& output)
{
    return false;
}

#undef CheckTypeError

MAA_RES_NS_END
