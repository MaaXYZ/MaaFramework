#include "PipelineResMgr.h"

#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

bool PipelineResMgr::load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        clear();
    }

    paths_.emplace_back(path);

    if (!load_all_json(path)) {
        LogError << "load_all_json failed" << VAR(path);
        return false;
    }

    if (!check_all_next_list()) {
        LogError << "check_all_next_list failed" << VAR(path);
        return false;
    }

    return true;
}

void PipelineResMgr::clear()
{
    LogFunc;

    task_data_map_.clear();
    paths_.clear();
}

const TaskData& PipelineResMgr::get_task_data(const std::string& task_name)
{
    auto task_iter = task_data_map_.find(task_name);
    if (task_iter == task_data_map_.end()) {
        LogError << "Invalid task name" << VAR(task_name);
        static TaskData empty;
        return empty;
    }

    return task_iter->second;
}

bool PipelineResMgr::load_all_json(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        LogError << "path not exists";
        return false;
    }

    if (!std::filesystem::is_directory(path)) {
        LogError << "path is not directory" << VAR(path);
        return false;
    }

    bool valid = false;

    std::set<std::string> existing_keys;
    for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        auto& entry_path = entry.path();
        if (entry.is_directory()) {
            LogDebug << "entry is directory" << VAR(entry_path);
            continue;
        }
        if (!entry.is_regular_file()) {
            LogWarn << "entry is not regular file, skip" << VAR(entry_path);
            continue;
        }

        auto ext = path_to_utf8_string(entry_path.extension());
        tolowers_(ext);
        if (ext != ".json") {
            LogWarn << "entry is not *.json, skip" << VAR(entry_path) << VAR(ext);
            continue;
        }

        bool parsed = open_and_parse_file(entry_path, existing_keys);
        if (!parsed) {
            LogError << "open_and_parse_file failed" << VAR(entry_path);
            return false;
        }

        valid = true;
    }

    return valid;
}

bool PipelineResMgr::open_and_parse_file(
    const std::filesystem::path& path,
    std::set<std::string>& existing_keys)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }
    const auto& json = *json_opt;

    TaskDataMap cur_data_map;
    if (!parse_config(json, cur_data_map, existing_keys, task_data_map_)) {
        LogError << "parse_config failed" << VAR(path) << VAR(json);
        return false;
    }

    cur_data_map.merge(std::move(task_data_map_));
    task_data_map_ = std::move(cur_data_map);

    return true;
}

bool PipelineResMgr::check_all_next_list() const
{
    LogFunc;

    for (const auto& [name, task_data] : task_data_map_) {
        bool ret = check_next_list(task_data.next) && check_next_list(task_data.timeout_next)
                   && check_next_list(task_data.runout_next);
        if (!ret) {
            LogError << "check_next_list failed" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineResMgr::check_next_list(const TaskData::NextList& next_list) const
{
    for (const std::string& name : next_list) {
        if (!task_data_map_.contains(name)) {
            LogError << "Invalid next task name" << VAR(name);
            return false;
        }
    }
    return true;
}

std::vector<std::string> PipelineResMgr::get_task_list() const
{
    auto k = task_data_map_ | std::views::keys;
    return std::vector(k.begin(), k.end());
}

bool PipelineResMgr::parse_config(
    const json::value& input,
    TaskDataMap& output,
    std::set<std::string>& existing_keys,
    const TaskDataMap& default_value)
{
    if (!input.is_object()) {
        LogError << "json is not object";
        return false;
    }

    TaskDataMap data_map;

    for (const auto& [key, value] : input.as_object()) {
        if (key.empty()) {
            LogError << "key is empty" << VAR(key);
            return false;
        }
        if (key.starts_with('$')) {
            LogInfo << "key starts with '$', skip" << VAR(key);
            continue;
        }
        if (existing_keys.contains(key)) {
            LogError << "key already exists" << VAR(key);
            return false;
        }
        if (!value.is_object()) {
            LogError << "value is not object" << VAR(key) << VAR(value);
            return false;
        }

        TaskData task_data;
        const auto& default_task_data =
            default_value.contains(key) ? default_value.at(key) : TaskData {};
        bool ret = parse_task(key, value, task_data, default_task_data);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }
        data_map.insert_or_assign(key, task_data);
        existing_keys.emplace(key);
    }

    output = std::move(data_map);

    return true;
}

template <typename OutT>
bool get_and_check_value(
    const json::value& input,
    const std::string& key,
    OutT& output,
    const OutT& default_val)
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
bool get_and_check_value_or_array(
    const json::value& input,
    const std::string& key,
    std::vector<OutT>& output,
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

bool PipelineResMgr::parse_task(
    const std::string& name,
    const json::value& input,
    TaskData& output,
    const TaskData& default_value)
{
    LogTrace << VAR(name);

    TaskData data;
    data.name = name;

    if (!get_and_check_value(input, "is_sub", data.is_sub, default_value.is_sub)) {
        LogError << "failed to get_and_check_value is_sub" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "inverse", data.inverse, default_value.inverse)) {
        LogError << "failed to get_and_check_value inverse" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "enabled", data.enabled, default_value.enabled)) {
        LogError << "failed to get_and_check_value enabled" << VAR(input);
        return false;
    }

    if (!parse_recognition(
            input,
            data.rec_type,
            data.rec_param,
            default_value.rec_type,
            default_value.rec_param)) {
        LogError << "failed to parse_recognition" << VAR(input);
        return false;
    }

    if (!parse_action(
            input,
            data.action_type,
            data.action_param,
            default_value.action_type,
            default_value.action_param)) {
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

    if (!get_and_check_value_or_array(
            input,
            "timeout_next",
            data.timeout_next,
            default_value.timeout_next)) {
        LogError << "failed to parse_next timeout_next" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "times_limit", data.times_limit, default_value.times_limit)) {
        LogError << "failed to get_and_check_value times_limit" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(
            input,
            "runout_next",
            data.runout_next,
            default_value.runout_next)) {
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

    if (!parse_wait_freezes_param(
            input,
            "pre_wait_freezes",
            data.pre_wait_freezes,
            default_value.pre_wait_freezes)) {
        LogError << "failed to pre_wait_freezes" << VAR(input);
        return false;
    }

    if (!parse_wait_freezes_param(
            input,
            "post_wait_freezes",
            data.post_wait_freezes,
            default_value.post_wait_freezes)) {
        LogError << "failed to post_wait_freezes" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "focus", data.focus, default_value.focus)) {
        LogError << "failed to get_and_check_value focus" << VAR(input);
        return false;
    }

    output = std::move(data);

    return true;
}

bool PipelineResMgr::parse_recognition(
    const json::value& input,
    Recognition::Type& out_type,
    Recognition::Param& out_param,
    const Recognition::Type& default_type,
    const Recognition::Param& default_param)
{
    using namespace Recognition;
    using namespace MAA_VISION_NS;

    static const std::string kDefaultRecognitionFlag = "Default";
    std::string rec_type_name;
    if (!get_and_check_value(input, "recognition", rec_type_name, kDefaultRecognitionFlag)
        && !get_and_check_value(
            input,
            "recognizer",
            rec_type_name,
            kDefaultRecognitionFlag)) { // for compatibility
        LogError << "failed to get_and_check_value recognition" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, Type> kRecTypeMap = {
        { kDefaultRecognitionFlag, default_type },
        { "DirectHit", Type::DirectHit },
        { "TemplateMatch", Type::TemplateMatch },
        { "FeatureMatch", Type::FeatureMatch },
        { "OCR", Type::OCR },
        { "NeuralNetworkClassify", Type::NeuralNetworkClassify },
        { "NeuralNetworkDetect", Type::NeuralNetworkDetect },
        { "ColorMatch", Type::ColorMatch },
        { "Custom", Type::Custom },
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
        out_param = DirectHitParam {};
        return true;
        // return parse_direct_hit_param(input, std::get<DirectHitParam>(out_param),
        //                               same_type ? std::get<DirectHitParam>(default_param) :
        //                               DirectHitParam {});

    case Type::TemplateMatch:
        out_param = TemplateMatcherParam {};
        return parse_template_matcher_param(
            input,
            std::get<TemplateMatcherParam>(out_param),
            same_type ? std::get<TemplateMatcherParam>(default_param) : TemplateMatcherParam {});

    case Type::FeatureMatch:
        out_param = FeatureMatcherParam {};
        return parse_feature_matcher_param(
            input,
            std::get<FeatureMatcherParam>(out_param),
            same_type ? std::get<FeatureMatcherParam>(default_param) : FeatureMatcherParam {});

    case Type::NeuralNetworkClassify:
        out_param = NeuralNetworkClassifierParam {};
        return parse_nn_classifier_param(
            input,
            std::get<NeuralNetworkClassifierParam>(out_param),
            same_type ? std::get<NeuralNetworkClassifierParam>(default_param)
                      : NeuralNetworkClassifierParam {});

    case Type::NeuralNetworkDetect:
        out_param = NeuralNetworkDetectorParam {};
        return parse_nn_detector_param(
            input,
            std::get<NeuralNetworkDetectorParam>(out_param),
            same_type ? std::get<NeuralNetworkDetectorParam>(default_param)
                      : NeuralNetworkDetectorParam {});

    case Type::OCR:
        out_param = OCRerParam {};
        return parse_ocrer_param(
            input,
            std::get<OCRerParam>(out_param),
            same_type ? std::get<OCRerParam>(default_param) : OCRerParam {});

    case Type::ColorMatch:
        out_param = ColorMatcherParam {};
        return parse_color_matcher_param(
            input,
            std::get<ColorMatcherParam>(out_param),
            same_type ? std::get<ColorMatcherParam>(default_param) : ColorMatcherParam {});

    case Type::Custom:
        out_param = CustomRecognizerParam {};
        return parse_custom_recognition_param(
            input,
            std::get<CustomRecognizerParam>(out_param),
            same_type ? std::get<CustomRecognizerParam>(default_param) : CustomRecognizerParam {});
    default:
        LogError << "Unknown recognition" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

// bool PipelineResMgr::parse_direct_hit_param(const json::value& input,
// MAA_VISION_NS::DirectHitParam& output,
//                                             const MAA_VISION_NS::DirectHitParam& default_value)
//{
//     // if (!parse_roi(input, output.roi, default_value.roi)) {
//     //     LogError << "failed to parse_roi" << VAR(input);
//     //     return false;
//     // }
//
//     return true;
// }

bool PipelineResMgr::parse_template_matcher_param(
    const json::value& input,
    MAA_VISION_NS::TemplateMatcherParam& output,
    const MAA_VISION_NS::TemplateMatcherParam& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!parse_order_of_result(
            input,
            output.order_by,
            output.result_index,
            default_value.order_by,
            default_value.result_index,
            {
                MAA_VISION_NS::ResultOrderBy::Horizontal,
                MAA_VISION_NS::ResultOrderBy::Vertical,
                MAA_VISION_NS::ResultOrderBy::Score,
                MAA_VISION_NS::ResultOrderBy::Random,
            })) {
        LogError << "failed to parse_order_of_result" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(
            input,
            "template",
            output.template_paths,
            default_value.template_paths)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
        return false;
    }
    if (output.template_paths.empty()) {
        LogError << "templates is empty" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(
            input,
            "threshold",
            output.thresholds,
            default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
        return false;
    }

    if (output.thresholds.empty()) {
        output.thresholds = std::vector(
            output.template_paths.size(),
            MAA_VISION_NS::TemplateMatcherParam::kDefaultThreshold);
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

bool PipelineResMgr::parse_feature_matcher_param(
    const json::value& input,
    MAA_VISION_NS::FeatureMatcherParam& output,
    const MAA_VISION_NS::FeatureMatcherParam& default_value)
{
    using namespace MAA_VISION_NS;

    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!parse_order_of_result(
            input,
            output.order_by,
            output.result_index,
            default_value.order_by,
            default_value.result_index,
            {
                MAA_VISION_NS::ResultOrderBy::Horizontal,
                MAA_VISION_NS::ResultOrderBy::Vertical,
                MAA_VISION_NS::ResultOrderBy::Score,
                MAA_VISION_NS::ResultOrderBy::Area,
                MAA_VISION_NS::ResultOrderBy::Random,
            })) {
        LogError << "failed to parse_order_of_result" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(
            input,
            "template",
            output.template_paths,
            default_value.template_paths)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
        return false;
    }
    if (output.template_paths.empty()) {
        LogError << "templates is empty" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "green_mask", output.green_mask, default_value.green_mask)) {
        LogError << "failed to get_and_check_value green_mask" << VAR(input);
        return false;
    }

    static const std::string kDefaultDetectorFlag = "Default";
    std::string detector;
    if (!get_and_check_value(input, "detector", detector, kDefaultDetectorFlag)) {
        LogError << "failed to get_and_check_value detector" << VAR(input);
        return false;
    }
    const std::unordered_map<std::string, FeatureMatcherParam::Detector> kDetectorMap = {
        { kDefaultDetectorFlag, default_value.detector },
        { "SIFT", FeatureMatcherParam::Detector::SIFT },
        { "SURF", FeatureMatcherParam::Detector::SURF },
        { "ORB", FeatureMatcherParam::Detector::ORB },
        { "BRISK", FeatureMatcherParam::Detector::BRISK },
        { "KAZE", FeatureMatcherParam::Detector::KAZE },
        { "AKAZE", FeatureMatcherParam::Detector::AKAZE },
    };
    auto detector_iter = kDetectorMap.find(detector);
    if (detector_iter == kDetectorMap.end()) {
        LogError << "detector not found" << VAR(detector);
        return false;
    }
    output.detector = detector_iter->second;

#ifndef MAA_VISION_HAS_XFEATURES2D
    if (output.detector == FeatureMatcherParam::Detector::SURF) {
        LogError << "MAA build without xfeatures2d, SURF is not supported" << VAR(input);
        return false;
    }
#endif

    // if (!get_and_check_value(input, "detector_param", output.detector_param,
    // default_value.detector_param)) {
    //     LogError << "failed to get_and_check_value detector_param" << VAR(input);
    //     return false;
    // }

    if (!get_and_check_value(
            input,
            "distance_ratio",
            output.distance_ratio,
            default_value.distance_ratio)) {
        LogError << "failed to get_and_check_value distance_ratio" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "count", output.count, default_value.count)) {
        LogError << "failed to get_and_check_value count" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_ocrer_param(
    const json::value& input,
    MAA_VISION_NS::OCRerParam& output,
    const MAA_VISION_NS::OCRerParam& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!parse_order_of_result(
            input,
            output.order_by,
            output.result_index,
            default_value.order_by,
            default_value.result_index,
            {
                MAA_VISION_NS::ResultOrderBy::Horizontal,
                MAA_VISION_NS::ResultOrderBy::Vertical,
                MAA_VISION_NS::ResultOrderBy::Area,
                MAA_VISION_NS::ResultOrderBy::Length,
                MAA_VISION_NS::ResultOrderBy::Random,
            })) {
        LogError << "failed to parse_order_of_result" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "model", output.model, default_value.model)) {
        LogError << "failed to get_and_check_value model" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "only_rec", output.only_rec, default_value.only_rec)) {
        LogError << "failed to get_and_check_value only_rec" << VAR(input);
        return false;
    }

    std::vector<std::string> u8_text;
    std::vector<std::string> u8_default_text;
    std::ranges::transform(default_value.expected, std::back_inserter(u8_default_text), from_u16);

    if (input.exists("expected")) {
        if (!get_and_check_value_or_array(input, "expected", u8_text, u8_default_text)) {
            LogError << "failed to get_and_check_value_or_array expected" << VAR(input);
            return false;
        }
    }
    // 已废弃字段，兼容一下
    else if (input.exists("text")) {
        if (!get_and_check_value_or_array(input, "text", u8_text, u8_default_text)) {
            LogError << "failed to get_and_check_value_or_array text" << VAR(input);
            return false;
        }
    }
    std::ranges::transform(u8_text, std::back_inserter(output.expected), to_u16);

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
            output.replace.emplace_back(to_u16(first.as_string()), to_u16(second.as_string()));
        }
    }
    else {
        output.replace = default_value.replace;
    }

    return true;
}

bool PipelineResMgr::parse_custom_recognition_param(
    const json::value& input,
    MAA_VISION_NS::CustomRecognizerParam& output,
    const MAA_VISION_NS::CustomRecognizerParam& default_value)
{
    if (!get_and_check_value(input, "custom_recognition", output.name, default_value.name)
        && !get_and_check_value(input, "custom_recognizer", output.name, default_value.name)) {
        LogError << "failed to get_and_check_value custom_recognition" << VAR(input);
        return false;
    }

    if (output.name.empty()) {
        LogError << "custom_recognition is empty" << VAR(input);
        return false;
    }

    output.custom_param =
        input.get("custom_recognition_param", input.get("custom_recognizer_param", json::object()));

    return true;
}

bool PipelineResMgr::parse_nn_classifier_param(
    const json::value& input,
    MAA_VISION_NS::NeuralNetworkClassifierParam& output,
    const MAA_VISION_NS::NeuralNetworkClassifierParam& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!parse_order_of_result(
            input,
            output.order_by,
            output.result_index,
            default_value.order_by,
            default_value.result_index,
            {
                MAA_VISION_NS::ResultOrderBy::Horizontal,
                MAA_VISION_NS::ResultOrderBy::Vertical,
                MAA_VISION_NS::ResultOrderBy::Score,
                MAA_VISION_NS::ResultOrderBy::Random,
            })) {
        LogError << "failed to parse_order_of_result" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "cls_size", output.cls_size, default_value.cls_size)) {
        LogError << "failed to get_and_check_value cls_size" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "labels", output.labels, default_value.labels)) {
        LogError << "failed to get_and_check_value_or_array labels" << VAR(input);
        return false;
    }
    if (output.labels.size() < output.cls_size) {
        LogDebug << "labels.size() < cls_size, fill 'Unknown'" << VAR(output.labels.size())
                 << VAR(output.cls_size);
        output.labels.resize(output.cls_size, "Unknown");
    }

    if (!get_and_check_value(input, "model", output.model, default_value.model)) {
        LogError << "failed to get_and_check_value model" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "expected", output.expected, default_value.expected)) {
        LogError << "failed to get_and_check_value_or_array expected" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_nn_detector_param(
    const json::value& input,
    MAA_VISION_NS::NeuralNetworkDetectorParam& output,
    const MAA_VISION_NS::NeuralNetworkDetectorParam& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    if (!parse_order_of_result(
            input,
            output.order_by,
            output.result_index,
            default_value.order_by,
            default_value.result_index,
            {
                MAA_VISION_NS::ResultOrderBy::Horizontal,
                MAA_VISION_NS::ResultOrderBy::Vertical,
                MAA_VISION_NS::ResultOrderBy::Score,
                MAA_VISION_NS::ResultOrderBy::Area,
                MAA_VISION_NS::ResultOrderBy::Random,
            })) {
        LogError << "failed to parse_order_of_result" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "cls_size", output.cls_size, default_value.cls_size)) {
        LogError << "failed to get_and_check_value cls_size" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "labels", output.labels, default_value.labels)) {
        LogError << "failed to get_and_check_value_or_array labels" << VAR(input);
        return false;
    }
    if (output.labels.size() < output.cls_size) {
        LogDebug << "labels.size() < cls_size, fill 'Unknown'" << VAR(output.labels.size())
                 << VAR(output.cls_size);
        output.labels.resize(output.cls_size, "Unknown");
    }

    if (!get_and_check_value(input, "model", output.model, default_value.model)) {
        LogError << "failed to get_and_check_value model" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "expected", output.expected, default_value.expected)) {
        LogError << "failed to get_and_check_value_or_array expected" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(
            input,
            "threshold",
            output.thresholds,
            default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
        return false;
    }
    if (output.thresholds.empty()) {
        output.thresholds = std::vector(
            output.expected.size(),
            MAA_VISION_NS::NeuralNetworkDetectorParam::kDefaultThreshold);
    }
    else if (output.expected.size() != output.thresholds.size()) {
        LogError << "templates.size() != thresholds.size()" << VAR(output.expected.size())
                 << VAR(output.thresholds.size());
        return false;
    }

    return true;
}

template <typename OutT>
bool get_and_check_array_or_2darray(
    const json::value& input,
    const std::string& key,
    std::vector<std::vector<OutT>>& output,
    const std::vector<std::vector<OutT>>& default_value)
{
    auto opt = input.find(key);
    if (!opt) {
        output = default_value;
        return true;
    }
    if (!opt->is_array() || opt->as_array().empty()) {
        LogError << "type error" << VAR(key) << VAR(input);
        return false;
    }

    output.clear();

    auto& front = opt->as_array()[0];
    if (front.is_array()) {
        for (const auto& arr : opt->as_array()) {
            if (!arr.is_array()) {
                LogError << "type error" << VAR(key) << VAR(input);
                return false;
            }
            std::vector<OutT> row;
            for (const auto& item : arr.as_array()) {
                if (!item.is<OutT>()) {
                    LogError << "type error" << VAR(key) << VAR(input);
                    return false;
                }
                row.emplace_back(item.as<OutT>());
            }
            output.emplace_back(std::move(row));
        }
    }
    else if (front.is<OutT>()) {
        std::vector<OutT> row;
        for (const auto& item : opt->as_array()) {
            if (!item.is<OutT>()) {
                LogError << "type error" << VAR(key) << VAR(input);
                return false;
            }
            row.emplace_back(item.as<OutT>());
        }
        output.emplace_back(std::move(row));
    }
    else {
        LogError << "type error" << VAR(key) << VAR(input);
        return false;
    }

    return !output.empty();
}

bool PipelineResMgr::parse_color_matcher_param(
    const json::value& input,
    MAA_VISION_NS::ColorMatcherParam& output,
    const MAA_VISION_NS::ColorMatcherParam& default_value)
{
    if (!parse_roi(input, output.roi, default_value.roi)) {
        LogError << "failed to parse_roi" << VAR(input);
        return false;
    }

    std::vector<std::vector<int>> default_lower;
    std::vector<std::vector<int>> default_upper;

    for (const auto& pair : default_value.range) {
        default_lower.emplace_back(pair.first);
        default_upper.emplace_back(pair.second);
    }

    std::vector<std::vector<int>> lower;
    if (!get_and_check_array_or_2darray(input, "lower", lower, default_lower)) {
        LogError << "failed to get_and_check_array_or_2darray lower" << VAR(input);
        return false;
    }

    std::vector<std::vector<int>> upper;
    if (!get_and_check_array_or_2darray(input, "upper", upper, default_upper)) {
        LogError << "failed to get_and_check_array_or_2darray lower" << VAR(input);
        return false;
    }

    constexpr int kMaxChannel = 4;
    if (lower.empty() || lower.size() != upper.size()) {
        LogError << "bad size" << VAR(lower.size()) << VAR(upper.size());
        return false;
    }

    for (size_t i = 0; i != lower.size(); ++i) {
        auto& l = lower[i];
        auto& u = upper[i];
        if (l.empty() || l.size() != u.size() || l.size() > kMaxChannel) {
            LogError << "bad size" << VAR(l.size()) << VAR(u.size()) << VAR(kMaxChannel);
            return false;
        }
        output.range.emplace_back(std::make_pair(l, u));
    }

    if (!get_and_check_value(input, "count", output.count, default_value.count)) {
        LogError << "failed to get_and_check_value count" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "method", output.method, default_value.method)) {
        LogError << "failed to get_and_check_value method" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "connected", output.connected, default_value.connected)) {
        LogError << "failed to get_and_check_value connected" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_roi(
    const json::value& input,
    std::vector<cv::Rect>& output,
    const std::vector<cv::Rect>& default_value)
{
    auto roi_opt = input.find("roi");
    if (!roi_opt) {
        output = default_value;
        return true;
    }
    if (!roi_opt->is_array()) {
        LogError << "roi is not array" << VAR(input);
        return false;
    }
    auto& roi_array = roi_opt->as_array();
    if (roi_array.empty()) {
        LogError << "roi array is empty" << VAR(input);
        return false;
    }

    if (roi_array.at(0).is_number()) {
        cv::Rect single_roi {};
        if (parse_rect(*roi_opt, single_roi)) {
            output = { single_roi };
            return true;
        }
        else {
            LogError << "failed to parse_rect for single_roi" << VAR(roi_array);
            return false;
        }
    }

    output.clear();
    for (const auto& roi_item : roi_array) {
        cv::Rect roi {};
        if (!parse_rect(roi_item, roi)) {
            LogError << "failed to parse_rect for multi_roi" << VAR(roi_item) << VAR(roi_array);
            return false;
        }
        output.emplace_back(roi);
    }

    return !output.empty();
}

bool PipelineResMgr::parse_order_of_result(
    const json::value& input,
    MAA_VISION_NS::ResultOrderBy& output,
    int& output_index,
    const MAA_VISION_NS::ResultOrderBy& default_value,
    int default_index,
    const std::unordered_set<MAA_VISION_NS::ResultOrderBy>& valid_values)
{
    static const std::string kDefaultOrderFlag = "Default";
    std::string order;
    if (!get_and_check_value(input, "order", order, kDefaultOrderFlag)) {
        LogError << "failed to get_and_check_value order" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, MAA_VISION_NS::ResultOrderBy> kOrderMap = {
        { kDefaultOrderFlag, default_value },
        { "Horizontal", MAA_VISION_NS::ResultOrderBy::Horizontal },
        { "Vertical", MAA_VISION_NS::ResultOrderBy::Vertical },
        { "Score", MAA_VISION_NS::ResultOrderBy::Score },
        { "Area", MAA_VISION_NS::ResultOrderBy::Area },
        { "Length", MAA_VISION_NS::ResultOrderBy::Length },
        { "Random", MAA_VISION_NS::ResultOrderBy::Random },
        { "Expected", MAA_VISION_NS::ResultOrderBy::Expected },
    };
    auto order_iter = kOrderMap.find(order);
    if (order_iter == kOrderMap.end()) {
        LogError << "order not found" << VAR(order);
        return false;
    }
    if (!valid_values.contains(order_iter->second)) {
        LogError << "current recognition not support order" << VAR(order);
        return false;
    }
    output = order_iter->second;

    if (!get_and_check_value(input, "index", output_index, default_index)) {
        LogError << "failed to get_and_check_value index" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_action(
    const json::value& input,
    Action::Type& out_type,
    Action::Param& out_param,
    const Action::Type& default_type,
    const Action::Param& default_param)
{
    using namespace Action;

    static const std::string kDefaultActionFlag = "Default";
    std::string act_type_name;
    if (!get_and_check_value(input, "action", act_type_name, kDefaultActionFlag)) {
        LogError << "failed to get_and_check_value action" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, Type> kActTypeMap = {
        { kDefaultActionFlag, default_type },
        { "DoNothing", Type::DoNothing },
        { "Click", Type::Click },
        { "Swipe", Type::Swipe },
        { "Key", Type::Key },
        { "InputText", Type::Text },
        { "Text", Type::Text },
        { "StartApp", Type::StartApp },
        { "StopApp", Type::StopApp },
        { "Custom", Type::Custom },
        { "StopTask", Type::StopTask },
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
        out_param = ClickParam {};
        return parse_click(
            input,
            std::get<ClickParam>(out_param),
            same_type ? std::get<ClickParam>(default_param) : ClickParam {});
    case Type::Swipe:
        out_param = SwipeParam {};
        return parse_swipe(
            input,
            std::get<SwipeParam>(out_param),
            same_type ? std::get<SwipeParam>(default_param) : SwipeParam {});

    case Type::Key:
        out_param = KeyParam {};
        return parse_press_key(
            input,
            std::get<KeyParam>(out_param),
            same_type ? std::get<KeyParam>(default_param) : KeyParam {});
    case Type::Text:
        out_param = TextParam {};
        return parse_input_text(
            input,
            std::get<TextParam>(out_param),
            same_type ? std::get<TextParam>(default_param) : TextParam {});
    case Type::StartApp:
    case Type::StopApp:
        out_param = AppParam {};
        return parse_app_info(
            input,
            std::get<AppParam>(out_param),
            same_type ? std::get<AppParam>(default_param) : AppParam {});

    case Type::Custom:
        out_param = CustomParam {};
        return parse_custom_action_param(
            input,
            std::get<CustomParam>(out_param),
            same_type ? std::get<CustomParam>(default_param) : CustomParam {});

    case Type::StopTask:
        out_param = {};
        return true;

    default:
        LogError << "unknown act type" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineResMgr::parse_click(
    const json::value& input,
    Action::ClickParam& output,
    const Action::ClickParam& default_value)
{
    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_swipe(
    const json::value& input,
    Action::SwipeParam& output,
    const Action::SwipeParam& default_value)
{
    if (!parse_action_target(input, "begin", output.begin, default_value.begin)) {
        LogError << "failed to parse_action_target begin" << VAR(input);
        return false;
    }

    if (!parse_action_target(input, "end", output.end, default_value.end)) {
        LogError << "failed to parse_action_target end" << VAR(input);
        return false;
    }
    if (output.begin.type == Action::Target::Type::Self
        && output.end.type == Action::Target::Type::Self) {
        LogError << "not set swipe begin or end";
        return false;
    }

    if (!get_and_check_value(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_press_key(
    const json::value& input,
    Action::KeyParam& output,
    const Action::KeyParam& default_value)
{
    // TODO: https://github.com/MaaXYZ/MaaFramework/issues/24#issuecomment-1666533842
    if (!get_and_check_value_or_array(input, "key", output.keys, default_value.keys)) {
        LogError << "failed to get_and_check_value_or_array key" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_input_text(
    const json::value& input,
    Action::TextParam& output,
    const Action::TextParam& default_value)
{
    if (!get_and_check_value(input, "input_text", output.text, default_value.text)) {
        LogError << "failed to get_and_check_value text" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_app_info(
    const json::value& input,
    Action::AppParam& output,
    const Action::AppParam& default_value)
{
    if (!get_and_check_value(input, "package", output.package, default_value.package)) {
        LogError << "failed to get_and_check_value activity" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_custom_action_param(
    const json::value& input,
    Action::CustomParam& output,
    const Action::CustomParam& default_value)
{
    if (!get_and_check_value(input, "custom_action", output.name, default_value.name)) {
        LogError << "failed to get_and_check_value custom_action" << VAR(input);
        return false;
    }

    if (output.name.empty()) {
        LogError << "custom_action is empty" << VAR(input);
        return false;
    }

    output.custom_param = input.get("custom_action_param", json::object());

    return true;
}

bool PipelineResMgr::parse_wait_freezes_param(
    const json::value& input,
    const std::string& key,
    WaitFreezesParam& output,
    const WaitFreezesParam& default_value)
{
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
            LogError << "failed to parse_wait_freezes_param time" << VAR(field);
            return false;
        }
        output.time = std::chrono::milliseconds(time);

        if (!parse_action_target(field, "target", output.target, default_value.target)) {
            LogError << "failed to parse_wait_freezes_param parse_action_target" << VAR(field);
            return false;
        }

        if (!get_and_check_value(field, "threshold", output.threshold, default_value.threshold)) {
            LogError << "failed to parse_wait_freezes_param threshold" << VAR(field);
            return false;
        }

        if (!get_and_check_value(field, "method", output.method, default_value.method)) {
            LogError << "failed to parse_wait_freezes_param method" << VAR(field);
            return false;
        }
        return true;
    }
    else {
        LogError << "invalid wait_freezes_param" << VAR(field);
        return false;
    }
}

bool PipelineResMgr::parse_rect(const json::value& input_rect, cv::Rect& output)
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

bool PipelineResMgr::parse_action_target(
    const json::value& input,
    const std::string& key,
    Action::Target& output,
    const Action::Target& default_value)
{
    using namespace Action;

    if (auto param_opt = input.find(key); !param_opt) {
        output = default_value;
    }
    else if (param_opt->is_boolean() && param_opt->as_boolean()) {
        output.type = Target::Type::Self;
    }
    else if (param_opt->is_string()) {
        output.type = Target::Type::PreTask;
        output.param = param_opt->as_string();
    }
    else if (param_opt->is_array()) {
        output.type = Target::Type::Region;
        cv::Rect rect {};
        if (!parse_rect(*param_opt, rect)) {
            LogError << "Target::Type::Region failed to parse_rect" << VAR(*param_opt);
            return false;
        }
        output.param = rect;
    }
    else {
        LogError << "param type error" << VAR(key) << VAR(*param_opt);
        return false;
    }

    if (auto offset_opt = input.find(key + "_offset"); !offset_opt) {
        output.offset = default_value.offset;
    }
    else if (offset_opt->is_array()) {
        if (!parse_rect(*offset_opt, output.offset)) {
            LogError << "failed to parse_rect" << key + "_offset" << VAR(*offset_opt);
            return false;
        }
    }
    else {
        LogError << "offset type error" << VAR(key + "_offset") << VAR(*offset_opt);
        return false;
    }

    return true;
}

MAA_RES_NS_END