#include "PipelineConfig.h"

#include "MaaUtils/Logger.hpp"
#include "Vision/VisionTypes.h"

#include <tuple>

MAA_RES_NS_BEGIN

using namespace MAA_PIPELINE_RES_NS;

bool PipelineConfig::load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        clear();
    }

    bool loaded = load_all_json(path);
    loaded &= load_template_images(path);
    loaded &= check_all_next_list();

    return loaded;
}

void PipelineConfig::clear()
{
    LogFunc;

    task_data_map_.clear();
}

const MAA_PIPELINE_RES_NS::TaskData& PipelineConfig::get_task_data(const std::string& task_name)
{
    auto task_iter = task_data_map_.find(task_name);
    if (task_iter == task_data_map_.end()) {
        LogError << "Invalid task name" << VAR(task_name);
        static MAA_PIPELINE_RES_NS::TaskData empty;
        return empty;
    }

    auto& task_data = task_iter->second;
    if (task_data.rec_type == MAA_PIPELINE_RES_NS::Recognition::Type::TemplateMatch) {
        auto& images = std::get<MAA_VISION_NS::TemplMatchingParams>(task_data.rec_params).template_images;
        if (images.empty()) {
            images = template_mgr_.get_template_images(task_name);
        }
    }

    return task_data;
}

bool PipelineConfig::load_all_json(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        LogError << "path not exists";
        return false;
    }

    bool loaded = false;
    if (std::filesystem::is_directory(path)) {
        for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") {
                continue;
            }
            loaded = open_and_parse_file(entry.path());
            if (!loaded) {
                LogError << "open_and_parse_file failed" << VAR(entry.path());
                return false;
            }
        }
    }
    else if (std::filesystem::is_regular_file(path) && path.extension() == ".json") {
        loaded = open_and_parse_file(path);
        if (!loaded) {
            LogError << "open_and_parse_file failed" << VAR(path);
            return false;
        }
    }
    else {
        LogError << "path is not directory or regular file";
        return false;
    }

    return loaded;
}

bool PipelineConfig::open_and_parse_file(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }

    TaskDataMap cur_data_map;
    if (!parse_json(*json_opt, cur_data_map, task_data_map_)) {
        return false;
    }

    cur_data_map.merge(std::move(task_data_map_));
    task_data_map_ = std::move(cur_data_map);

    return true;
}

bool PipelineConfig::load_template_images(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    for (const auto& [name, task_data] : task_data_map_) {
        if (task_data.rec_type != MAA_PIPELINE_RES_NS::Recognition::Type::TemplateMatch) {
            continue;
        }
        const auto& relatives = std::get<MAA_VISION_NS::TemplMatchingParams>(task_data.rec_params).template_paths;
        std::vector<std::filesystem::path> paths;
        ranges::transform(relatives, std::back_inserter(paths),
                          [&](const std::string& rlt) { return path / MAA_NS::path(rlt); });
        bool ret = template_mgr_.lazy_load(name, paths);
        if (!ret) {
            LogError << "template_cfg_.lazy_load failed" << VAR(name) << VAR(paths);
            return false;
        }
    }
    return true;
}

bool PipelineConfig::check_all_next_list() const
{
    LogFunc;

    for (const auto& [name, task_data] : task_data_map_) {
        bool ret = check_next_list(task_data.next) && check_next_list(task_data.timeout_next) &&
                   check_next_list(task_data.runout_next);
        if (!ret) {
            LogError << "check_next_list failed" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineConfig::check_next_list(const TaskData::NextList& next_list) const
{
    for (const std::string& name : next_list) {
        if (!task_data_map_.contains(name)) {
            LogError << "Invalid next task name" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineConfig::parse_json(const json::value& input, TaskDataMap& output, const TaskDataMap& default_value)
{
    if (!input.is_object()) {
        LogError << "json is not object";
        return false;
    }

    TaskDataMap data_map;

    for (const auto& [key, value] : input.as_object()) {
        TaskData task_data;
        const auto& default_task_data = default_value.contains(key) ? default_value.at(key) : TaskData {};
        bool ret = parse_task(key, value, task_data, default_task_data);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }
        data_map.insert_or_assign(key, task_data);
    }

    output = std::move(data_map);

    return true;
}

template <typename OutT>
bool get_and_check_value(const json::value& input, const std::string& key, OutT& output, const OutT& default_val)
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

template <typename OutT>
bool get_and_check_value_or_array(const json::value& input, const std::string& key, std::vector<OutT>& output,
                                  const std::vector<OutT>& default_value)
{
    auto opt = input.find(key);
    if (!opt) {
        output = default_value;
        return true;
    }

    if (opt->is_array()) {
        output.clear();
        for (const auto& item : opt->as_array()) {
            if (!item.is<OutT>()) {
                LogError << "type error" << VAR(key) << VAR(input);
                return false;
            }
            output.emplace_back(item.as<OutT>());
        }
    }
    else if (opt->is<OutT>()) {
        output = { opt->as<OutT>() };
    }
    else {
        LogError << "type error" << VAR(key) << VAR(input);
        return false;
    }

    return !output.empty();
}

bool PipelineConfig::parse_task(const std::string& name, const json::value& input, TaskData& output,
                                const MAA_PIPELINE_RES_NS::TaskData& default_value)
{
    LogFunc << VAR(name);

    TaskData data;
    data.name = name;

    if (!get_and_check_value(input, "is_sub", data.is_sub, default_value.is_sub)) {
        LogError << "failed to get_and_check_value is_sub" << VAR(input);
        return false;
    }

    if (!parse_recognition(input, data.rec_type, data.rec_params, default_value.rec_type, default_value.rec_params)) {
        LogError << "failed to parse_recognition" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "cache", data.cache, default_value.cache)) {
        LogError << "failed to get_and_check_value cache" << VAR(input);
        return false;
    }

    if (!parse_action(input, data.action_type, data.action_params, default_value.action_type,
                      default_value.action_params)) {
        LogError << "failed to parse_action" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "next", data.next, default_value.next)) {
        LogError << "failed to parse_next next" << VAR(input);
        return false;
    }

    auto timeout = default_value.timeout.count();
    if (!get_and_check_value(input, "timeout", timeout, timeout)) {
        LogError << "failed to get_and_check_value timeout" << VAR(input);
        return false;
    }
    data.timeout = std::chrono::milliseconds(timeout);

    if (!get_and_check_value_or_array(input, "timeout_next", data.timeout_next, default_value.timeout_next)) {
        LogError << "failed to parse_next timeout_next" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "times_limit", data.times_limit, default_value.times_limit)) {
        LogError << "failed to get_and_check_value times_limit" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "runout_next", data.runout_next, default_value.runout_next)) {
        LogError << "failed to parse_next runout_next" << VAR(input);
        return false;
    }

    auto pre_delay = default_value.pre_delay.count();
    if (!get_and_check_value(input, "pre_delay", pre_delay, pre_delay)) {
        LogError << "failed to get_and_check_value pre_delay" << VAR(input);
        return false;
    }
    data.pre_delay = std::chrono::milliseconds(pre_delay);

    auto post_delay = default_value.post_delay.count();
    if (!get_and_check_value(input, "post_delay", post_delay, post_delay)) {
        LogError << "failed to get_and_check_value post_delay" << VAR(input);
        return false;
    }
    data.post_delay = std::chrono::milliseconds(post_delay);

    if (!parse_wait_freezes_params(input, "pre_wait_freezes", data.pre_wait_freezes, default_value.pre_wait_freezes)) {
        LogError << "failed to pre_wait_freezes" << VAR(input);
        return false;
    }

    if (!parse_wait_freezes_params(input, "post_wait_freezes", data.post_wait_freezes,
                                   default_value.post_wait_freezes)) {
        LogError << "failed to post_wait_freezes" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "notify", data.notify, default_value.notify)) {
        LogError << "failed to get_and_check_value notify" << VAR(input);
        return false;
    }

    output = std::move(data);

    return true;
}

bool PipelineConfig::parse_recognition(const json::value& input, MAA_PIPELINE_RES_NS::Recognition::Type& out_type,
                                       MAA_PIPELINE_RES_NS::Recognition::Params& out_param,
                                       const MAA_PIPELINE_RES_NS::Recognition::Type& default_type,
                                       const MAA_PIPELINE_RES_NS::Recognition::Params& default_param)
{
    using namespace MAA_PIPELINE_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    static const std::string kDefaultRecognitionFlag = "DefaultRecognitionFlag";
    std::string rec_type_name;
    if (!get_and_check_value(input, "recognition", rec_type_name, kDefaultRecognitionFlag)) {
        LogError << "failed to get_and_check_value recognition" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, Type> kRecTypeMap = {
        { kDefaultRecognitionFlag, default_type },
        { "DirectHit", Type::DirectHit },
        { "TemplateMatch", Type::TemplateMatch },
        { "OCR", Type::OCR },
    };
    auto rec_type_iter = kRecTypeMap.find(rec_type_name);
    if (rec_type_iter == kRecTypeMap.end()) {
        LogError << "rec type not found" << VAR(rec_type_name);
        return false;
    }
    out_type = rec_type_iter->second;

    bool same_type = default_type == out_type;
    switch (out_type) {
    case Type::DirectHit:
        out_param = DirectHitParams {};
        return parse_direct_hit_params(input, std::get<DirectHitParams>(out_param),
                                       same_type ? std::get<DirectHitParams>(default_param) : DirectHitParams {});
    case Type::TemplateMatch:
        out_param = TemplMatchingParams {};
        return parse_templ_matching_params(input, std::get<TemplMatchingParams>(out_param),
                                           same_type ? std::get<TemplMatchingParams>(default_param)
                                                     : TemplMatchingParams {});
    case Type::OCR:
        out_param = OcrParams {};
        return parse_ocr_params(input, std::get<OcrParams>(out_param),
                                same_type ? std::get<OcrParams>(default_param) : OcrParams {});
    default:
        return false;
    }

    return false;
}

bool PipelineConfig::parse_direct_hit_params(const json::value& input, MAA_VISION_NS::DirectHitParams& output,
                                             const MAA_VISION_NS::DirectHitParams& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_templ_matching_params(const json::value& input, MAA_VISION_NS::TemplMatchingParams& output,
                                                 const MAA_VISION_NS::TemplMatchingParams& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "template", output.template_paths, default_value.template_paths)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
        return false;
    }
    if (output.template_paths.empty()) {
        LogError << "templates is empty" << VAR(input);
        return false;
    }
    if (output.template_paths == default_value.template_paths) {
        output.template_images = default_value.template_images;
    }

    if (!get_and_check_value_or_array(input, "threshold", output.thresholds, default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
        return false;
    }

    if (output.thresholds.empty()) {
        output.thresholds =
            std::vector(output.template_paths.size(), MAA_VISION_NS::TemplMatchingParams::kDefaultThreshold);
    }
    else if (output.template_paths.size() != output.thresholds.size()) {
        LogError << "templates.size() != thresholds.size()" << VAR(output.template_paths.size())
                 << VAR(output.thresholds.size());
        return false;
    }

    if (!get_and_check_value(input, "method", output.method, default_value.method)) {
        LogError << "failed to get_and_check_value method" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "green_mask", output.green_mask, default_value.green_mask)) {
        LogError << "failed to get_and_check_value green_mask" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_ocr_params(const json::value& input, MAA_VISION_NS::OcrParams& output,
                                      const MAA_VISION_NS::OcrParams& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "only_rec", output.only_rec, default_value.only_rec)) {
        LogError << "failed to get_and_check_value only_rec" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "text", output.text, default_value.text)) {
        LogError << "failed to get_and_check_value_or_array text" << VAR(input);
        return false;
    }

    if (auto replace_opt = input.find("replace")) {
        if (!replace_opt->is_array()) {
            LogError << "replace is not array" << VAR(input);
            return false;
        }
        auto& replace_array = replace_opt->as_array();
        for (const auto& pair : replace_array) {
            if (!pair.is_array()) {
                LogError << "replace pair is not array" << VAR(input);
                return false;
            }
            auto& pair_array = pair.as_array();
            if (pair_array.size() != 2) {
                LogError << "replace pair size != 2" << VAR(input);
                return false;
            }
            auto& first = pair_array[0];
            auto& second = pair_array[1];
            if (!first.is_string() || !second.is_string()) {
                LogError << "replace pair is not string" << VAR(input);
                return false;
            }
            output.replace.emplace_back(std::make_pair(first.as_string(), second.as_string()));
        }
    }
    else {
        output.replace = default_value.replace;
    }

    return true;
}

bool PipelineConfig::parse_roi(const json::value& input, std::vector<cv::Rect>& output,
                               const std::vector<cv::Rect>& default_value)
{
    auto roi_opt = input.find("roi");
    if (!roi_opt) {
        output = default_value;
        return true;
    }

    cv::Rect single_roi;
    if (parse_rect(*roi_opt, single_roi)) {
        output = { single_roi };
        return true;
    }

    if (!roi_opt->is_array()) {
        LogError << "roi is not array" << VAR(input);
        return false;
    }

    auto& roi_array = roi_opt->as_array();
    output.clear();
    for (const auto& roi_item : roi_array) {
        cv::Rect roi;
        if (!parse_rect(roi_item, roi)) {
            LogError << "failed to parse_rect" << VAR(roi_item);
            return false;
        }
        output.emplace_back(roi);
    }

    return !output.empty();
}

bool PipelineConfig::parse_action(const json::value& input, MAA_PIPELINE_RES_NS::Action::Type& out_type,
                                  MAA_PIPELINE_RES_NS::Action::Params& out_param,
                                  const MAA_PIPELINE_RES_NS::Action::Type& default_type,
                                  const MAA_PIPELINE_RES_NS::Action::Params& default_param)
{
    using namespace Action;

    static const std::string kDefaultActionFlag = "DefaultActionFlag";
    std::string act_type_name;
    if (!get_and_check_value(input, "action", act_type_name, kDefaultActionFlag)) {
        LogError << "failed to get_and_check_value action" << VAR(input);
        return false;
    }

    static const std::unordered_map<std::string, Type> kActTypeMap = {
        { kDefaultActionFlag, default_type },
        { "DoNothing", Type::DoNothing },
        { "Click", Type::Click },
        { "Swipe", Type::Swipe },
        { "Key", Type::Key },
        { "StartApp", Type::StartApp },
        { "StopApp", Type::StopApp },
        { "CustomTask", Type::CustomTask },
    };
    auto act_type_iter = kActTypeMap.find(act_type_name);
    if (act_type_iter == kActTypeMap.cend()) {
        LogError << "act type not found" << VAR(act_type_name);
        return false;
    }
    out_type = act_type_iter->second;

    bool same_type = default_type == out_type;
    switch (out_type) {
    case Type::DoNothing:
        return true;

    case Type::Click:
        out_param = ClickParams {};
        return parse_click(input, std::get<ClickParams>(out_param),
                           same_type ? std::get<ClickParams>(default_param) : ClickParams {});
    case Type::Swipe:
        out_param = SwipeParams {};
        return parse_swipe(input, std::get<SwipeParams>(out_param),
                           same_type ? std::get<SwipeParams>(default_param) : SwipeParams {});

    case Type::Key:
        out_param = KeyParams {};
        return parse_key_press(input, std::get<KeyParams>(out_param),
                               same_type ? std::get<KeyParams>(default_param) : KeyParams {});

    case Type::StartApp:
    case Type::StopApp:
        out_param = AppInfo {};
        return parse_app_info(input, std::get<AppInfo>(out_param),
                              same_type ? std::get<AppInfo>(default_param) : AppInfo {});

    case Type::CustomTask:
        out_param = CustomTaskParams {};
        return parse_custom_task_params(input, std::get<CustomTaskParams>(out_param),
                                        same_type ? std::get<CustomTaskParams>(default_param) : CustomTaskParams {});

    default:
        LogError << "unknown act type" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineConfig::parse_click(const json::value& input, MAA_PIPELINE_RES_NS::Action::ClickParams& output,
                                 const MAA_PIPELINE_RES_NS::Action::ClickParams& default_value)
{
    if (!parse_action_target(input, "target", output.target, output.target_param, default_value.target,
                             default_value.target_param)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_swipe(const json::value& input, MAA_PIPELINE_RES_NS::Action::SwipeParams& output,
                                 const MAA_PIPELINE_RES_NS::Action::SwipeParams& default_value)
{
    if (!parse_action_target(input, "begin", output.begin, output.begin_param, default_value.begin,
                             default_value.begin_param)) {
        LogError << "failed to parse_action_target begin" << VAR(input);
        return false;
    }

    if (!parse_action_target(input, "end", output.end, output.end_param, default_value.end, default_value.end_param)) {
        LogError << "failed to parse_action_target end" << VAR(input);
        return false;
    }
    if (output.begin == MAA_PIPELINE_RES_NS::Action::Target::Self &&
        output.end == MAA_PIPELINE_RES_NS::Action::Target::Self) {
        LogError << "not set swipe begin or end";
        return false;
    }

    if (!get_and_check_value(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_key_press(const json::value& input, MAA_PIPELINE_RES_NS::Action::KeyParams& output,
                                     const MAA_PIPELINE_RES_NS::Action::KeyParams& default_value)
{
    std::string str_keys;
    bool parse_str_ret = get_and_check_value(input, "key", str_keys, std::string());
    if (!parse_str_ret) {
        if (!get_and_check_value_or_array(input, "key", output.keys, default_value.keys)) {
            LogError << "failed to get_and_check_value_or_array key" << VAR(input);
            return false;
        }
    }
    else if (str_keys.empty()) {
        LogError << "key is empty" << VAR(input);
        return false;
    }
    else {
        ranges::transform(str_keys, std::back_inserter(output.keys), [](char c) { return static_cast<int>(c); });
        LogTrace << "key press" << VAR(str_keys) << VAR(output.keys);
    }

    return true;
}

bool PipelineConfig::parse_app_info(const json::value& input, MAA_PIPELINE_RES_NS::Action::AppInfo& output,
                                    const MAA_PIPELINE_RES_NS::Action::AppInfo& default_value)
{
    if (!get_and_check_value(input, "package", output.package, default_value.package)) {
        LogError << "failed to get_and_check_value activity" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineConfig::parse_custom_task_params(const json::value& input,
                                              MAA_PIPELINE_RES_NS::Action::CustomTaskParams& output,
                                              const MAA_PIPELINE_RES_NS::Action::CustomTaskParams& default_value)
{
    if (!get_and_check_value(input, "custom_task", output.task_name, default_value.task_name)) {
        LogError << "failed to get_and_check_value custom_param" << VAR(input);
        return false;
    }

    auto param_opt = input.find("custom_param");
    if (param_opt) {
        output.task_param = *param_opt;
    }

    return true;
}

bool PipelineConfig::parse_wait_freezes_params(const json::value& input, const std::string& key,
                                               MAA_PIPELINE_RES_NS::WaitFreezesParams& output,
                                               const MAA_PIPELINE_RES_NS::WaitFreezesParams& default_value)
{
    using namespace MAA_PIPELINE_RES_NS;

    auto opt = input.find(key);
    if (!opt) {
        output = default_value;
        return true;
    }

    const auto& field = *opt;

    if (field.is_number()) {
        output.time = std::chrono::milliseconds(field.as_unsigned());
        return true;
    }
    else if (field.is_object()) {
        auto time = default_value.time.count();
        if (!get_and_check_value(field, "time", time, time)) {
            LogError << "failed to parse_wait_freezes_params time" << VAR(field);
            return false;
        }
        output.time = std::chrono::milliseconds(time);

        if (!parse_action_target(field, "target", output.target, output.target_param, default_value.target,
                                 default_value.target_param)) {
            LogError << "failed to parse_wait_freezes_params parse_action_target" << VAR(field);
            return false;
        }

        if (!get_and_check_value(field, "threshold", output.threshold, default_value.threshold)) {
            LogError << "failed to parse_wait_freezes_params threshold" << VAR(field);
            return false;
        }

        if (!get_and_check_value(field, "method", output.method, default_value.method)) {
            LogError << "failed to parse_wait_freezes_params method" << VAR(field);
            return false;
        }
        return true;
    }
    else {
        LogError << "invalid wait_freezes_params" << VAR(field);
        return false;
    }
}

bool PipelineConfig::parse_rect(const json::value& input_rect, cv::Rect& output)
{
    if (!input_rect.is_array()) {
        LogError << "rect is not array" << VAR(input_rect);
        return false;
    }

    auto& rect_array = input_rect.as_array();
    if (rect_array.size() != 4) {
        LogError << "rect size != 4" << VAR(rect_array.size());
        return false;
    }

    std::vector<int> rect_move_vec;
    for (const auto& r : rect_array) {
        if (!r.is_number()) {
            LogError << "type error" << VAR(r) << "is not integer";
            return false;
        }
        rect_move_vec.emplace_back(r.as_integer());
    }
    output = cv::Rect(rect_move_vec[0], rect_move_vec[1], rect_move_vec[2], rect_move_vec[3]);
    return true;
}

bool PipelineConfig::parse_action_target(const json::value& input, const std::string& key,
                                         MAA_PIPELINE_RES_NS::Action::Target& output_type,
                                         MAA_PIPELINE_RES_NS::Action::TargetParam& output_param,
                                         const MAA_PIPELINE_RES_NS::Action::Target& default_type,
                                         const MAA_PIPELINE_RES_NS::Action::TargetParam& default_param)
{
    using namespace MAA_PIPELINE_RES_NS::Action;

    auto param_opt = input.find(key);
    if (!param_opt) {
        output_type = default_type;
        output_param = default_param;
    }
    else {
        if (param_opt->is_boolean() && param_opt->as_boolean()) {
            output_type = Target::Self;
        }
        else if (param_opt->is_string()) {
            output_type = Target::PreTask;
            output_param = param_opt->as_string();
        }
        else if (param_opt->is_array()) {
            output_type = Target::Region;
            output_param = cv::Rect();
            parse_rect(*param_opt, std::get<cv::Rect>(output_param));
        }
        else {
            LogError << "param type error" << VAR(*param_opt);
            return false;
        }
    }

    return true;
}

MAA_RES_NS_END
