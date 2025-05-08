#include "PipelineResMgr.h"

#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

bool PipelineResMgr::load(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr)
{
    LogFunc << VAR(path);

    if (!load_all_json(path, default_mgr)) {
        LogError << "load_all_json failed" << VAR(path);
        return false;
    }

    if (!check_all_validity(pipeline_data_map_)) {
        LogError << "check_all_validity failed" << VAR(path);
        return false;
    }

    paths_.emplace_back(path);

    return true;
}

void PipelineResMgr::clear()
{
    LogFunc;

    pipeline_data_map_.clear();
    paths_.clear();
}

bool PipelineResMgr::load_all_json(const std::filesystem::path& path, const DefaultPipelineMgr& default_mgr)
{
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
        auto relative = std::filesystem::relative(entry_path, path);
        for (const auto& part : relative) {
            if (part.string().starts_with('.')) {
                LogWarn << "entry starts with . skip" << VAR(entry_path) << VAR(part);
                continue;
            }
        }

        auto ext = path_to_utf8_string(entry_path.extension());
        tolowers_(ext);
        if (ext != ".json") {
            LogWarn << "entry is not *.json, skip" << VAR(entry_path) << VAR(ext);
            continue;
        }

        bool parsed = open_and_parse_file(entry_path, existing_keys, default_mgr);
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
    std::set<std::string>& existing_keys,
    const DefaultPipelineMgr& default_mgr)
{
    LogFunc << VAR(path);

    auto json_opt = json::open(path);
    if (!json_opt) {
        LogError << "json::open failed" << VAR(path);
        return false;
    }
    const auto& json = *json_opt;

    if (!json.is_object()) {
        LogError << "json is not object";
        return false;
    }

    if (!parse_and_override(json.as_object(), existing_keys, default_mgr)) {
        LogError << "parse_config failed" << VAR(path) << VAR(json);
        return false;
    }

    return true;
}

bool PipelineResMgr::check_all_validity(const PipelineDataMap& data_map)
{
    LogFunc;

    bool ret = check_all_next_list(data_map);
    ret &= check_all_regex(data_map);

    return ret;
}

bool PipelineResMgr::check_all_next_list(const PipelineDataMap& data_map)
{
    LogFunc;

    for (const auto& [name, pipeline_data] : data_map) {
        if (!check_next_list(pipeline_data.next, data_map)) {
            LogError << "check_next_list next failed" << VAR(name) << VAR(pipeline_data.next);
            return false;
        }
        if (!check_next_list(pipeline_data.interrupt, data_map)) {
            LogError << "check_next_list interrupt failed" << VAR(name) << VAR(pipeline_data.interrupt);
            return false;
        }
        if (!check_next_list(pipeline_data.on_error, data_map)) {
            LogError << "check_next_list on_error failed" << VAR(name) << VAR(pipeline_data.on_error);
            return false;
        }

        // 这里是由业务逻辑决定了这三个列表不应有重复元素，不代表以后有其他列表也要直接加进来
        std::set<std::string> all_next(pipeline_data.next.begin(), pipeline_data.next.end());
        all_next.insert(pipeline_data.interrupt.begin(), pipeline_data.interrupt.end());
        all_next.insert(pipeline_data.on_error.begin(), pipeline_data.on_error.end());

        if (all_next.size() != pipeline_data.next.size() + pipeline_data.interrupt.size() + pipeline_data.on_error.size()) {
            LogError << "there are duplicate elements in the next, interrupt and on_error" << VAR(name) << VAR(pipeline_data.next)
                     << VAR(pipeline_data.interrupt) << VAR(pipeline_data.on_error);
            return false;
        }
    }
    return true;
}

bool PipelineResMgr::check_all_regex(const PipelineDataMap& data_map)
{
    LogFunc;

    for (const auto& [name, pipeline_data] : data_map) {
        if (pipeline_data.reco_type != Recognition::Type::OCR) {
            continue;
        }
        const auto& reco_param = std::get<MAA_VISION_NS::OCRerParam>(pipeline_data.reco_param);
        bool valid = std::ranges::all_of(reco_param.expected, regex_valid)
                     && std::ranges::all_of(reco_param.replace | std::views::keys, regex_valid);
        if (!valid) {
            LogError << "regex invalid" << VAR(name);
            return false;
        }
    }
    return true;
}

bool PipelineResMgr::check_next_list(const PipelineData::NextList& next_list, const PipelineDataMap& data_map)
{
    for (const auto& next : next_list) {
        if (!data_map.contains(next)) {
            LogError << "Invalid next node name" << VAR(next);
            return false;
        }
    }
    return true;
}

std::vector<std::string> PipelineResMgr::get_node_list() const
{
    auto k = pipeline_data_map_ | std::views::keys;
    return std::vector(k.begin(), k.end());
}

bool PipelineResMgr::parse_and_override(const json::object& input, std::set<std::string>& existing_keys, const DefaultPipelineMgr& default_mgr)
{
    for (const auto& [key, value] : input) {
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

        PipelineData result;
        const auto& default_result = pipeline_data_map_.contains(key) ? pipeline_data_map_.at(key) : default_mgr.get_pipeline();
        bool ret = parse_node(key, value, result, default_result, default_mgr);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }

        existing_keys.emplace(key);
        pipeline_data_map_.insert_or_assign(key, std::move(result));
    }

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

// for compatibility
template <typename OutT>
bool get_multi_keys_and_check_value(const json::value& input, const std::vector<std::string>& keys, OutT& output, const OutT& default_val)
{
    for (const auto& k : keys) {
        auto opt = input.find<OutT>(k);
        if (!opt) {
            if (input.exists(k)) {
                LogError << "type error" << VAR(k) << VAR(input);
                return false;
            }
        }
        else {
            output = *opt;
            return true;
        }
    }

    output = default_val;
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

    return true;
}

bool PipelineResMgr::parse_node(
    const std::string& name,
    const json::value& input,
    PipelineData& output,
    const PipelineData& default_value,
    const DefaultPipelineMgr& default_mgr)
{
    LogDebug << VAR(name);

    PipelineData data;
    data.name = name;

    if (!get_and_check_value(input, "is_sub", data.is_sub, default_value.is_sub)) {
        LogError << "failed to get_and_check_value is_sub" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "inverse", data.inverse, default_value.inverse)) {
        LogError << "failed to get_and_check_value inverse" << VAR(input);
        return false;
    }

    if (!get_multi_keys_and_check_value(input, { "enable", "enabled" }, data.enable, default_value.enable)) {
        LogError << "failed to get_and_check_value enable" << VAR(input);
        return false;
    }

    if (!parse_recognition(input, data.reco_type, data.reco_param, default_value.reco_type, default_value.reco_param, default_mgr)) {
        LogError << "failed to parse_recognition" << VAR(input);
        return false;
    }

    if (!parse_action(input, data.action_type, data.action_param, default_value.action_type, default_value.action_param, default_mgr)) {
        LogError << "failed to parse_action" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "next", data.next, default_value.next)) {
        LogError << "failed to get_and_check_value_or_array next" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "interrupt", data.interrupt, default_value.interrupt)) {
        LogError << "failed to get_and_check_value_or_array interrupt" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "on_error", data.on_error, default_value.on_error)) {
        LogError << "failed to get_and_check_value_or_array on_error" << VAR(input);
        return false;
    }
    if (data.on_error.empty()) {
        // for compatibility with v1.x
        if (!get_and_check_value_or_array(input, "timeout_next", data.on_error, default_value.on_error)) {
            LogError << "failed to get_and_check_value_or_array timeout_next" << VAR(input);
            return false;
        }
    }

    auto rate_limit = default_value.rate_limit.count();
    if (!get_and_check_value(input, "rate_limit", rate_limit, rate_limit)) {
        LogError << "failed to get_and_check_value rate_limit" << VAR(input);
        return false;
    }
    data.rate_limit = std::chrono::milliseconds(rate_limit);

    auto timeout = default_value.reco_timeout.count();
    if (!get_and_check_value(input, "timeout", timeout, timeout)) {
        LogError << "failed to get_and_check_value timeout" << VAR(input);
        return false;
    }
    data.reco_timeout = std::chrono::milliseconds(timeout);

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

    if (!parse_wait_freezes_param(input, "pre_wait_freezes", data.pre_wait_freezes, default_value.pre_wait_freezes)) {
        LogError << "failed to pre_wait_freezes" << VAR(input);
        return false;
    }

    if (!parse_wait_freezes_param(input, "post_wait_freezes", data.post_wait_freezes, default_value.post_wait_freezes)) {
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
    const Recognition::Type& parent_type,
    const Recognition::Param& parent_param,
    const DefaultPipelineMgr& default_mgr)
{
    using namespace Recognition;
    using namespace MAA_VISION_NS;

    static const std::string kDefaultRecognitionFlag = "Default";
    std::string reco_type_name;
    if (!get_multi_keys_and_check_value(input, { "recognition", "algorithm" }, reco_type_name, kDefaultRecognitionFlag)) {
        LogError << "failed to get_and_check_value recognition" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, Type> kRecTypeMap = {
        { kDefaultRecognitionFlag, parent_type },
        { "DirectHit", Type::DirectHit },
        { "directhit", Type::DirectHit },
        { "TemplateMatch", Type::TemplateMatch },
        { "templatematch", Type::TemplateMatch },
        { "FeatureMatch", Type::FeatureMatch },
        { "featurematch", Type::FeatureMatch },
        { "OCR", Type::OCR },
        { "ocr", Type::OCR },
        { "NeuralNetworkClassify", Type::NeuralNetworkClassify },
        { "neuralnetworkclassify", Type::NeuralNetworkClassify },
        { "nnclassify", Type::NeuralNetworkClassify },
        { "NNClassify", Type::NeuralNetworkClassify },
        { "NeuralNetworkDetect", Type::NeuralNetworkDetect },
        { "neuralnetworkdetect", Type::NeuralNetworkDetect },
        { "NNDetect", Type::NeuralNetworkDetect },
        { "nnDetect", Type::NeuralNetworkDetect },
        { "ColorMatch", Type::ColorMatch },
        { "colormatch", Type::ColorMatch },
        { "Custom", Type::Custom },
        { "custom", Type::Custom },
    };
    auto reco_type_iter = kRecTypeMap.find(reco_type_name);
    if (reco_type_iter == kRecTypeMap.end()) {
        LogError << "rec type not found" << VAR(reco_type_name);
        return false;
    }
    out_type = reco_type_iter->second;

    bool same_type = parent_type == out_type;
    switch (out_type) {
    case Type::DirectHit:
        out_param = default_mgr.get_recognition_param<DirectHitParam>(Type::DirectHit);
        return true;

    case Type::TemplateMatch: {
        auto default_param = default_mgr.get_recognition_param<TemplateMatcherParam>(Type::TemplateMatch);
        out_param = default_param;
        return parse_template_matcher_param(
            input,
            std::get<TemplateMatcherParam>(out_param),
            same_type ? std::get<TemplateMatcherParam>(parent_param) : default_param);
    } break;

    case Type::FeatureMatch: {
        auto default_param = default_mgr.get_recognition_param<FeatureMatcherParam>(Type::FeatureMatch);
        out_param = default_param;
        return parse_feature_matcher_param(
            input,
            std::get<FeatureMatcherParam>(out_param),
            same_type ? std::get<FeatureMatcherParam>(parent_param) : default_param);
    } break;

    case Type::NeuralNetworkClassify: {
        auto default_param = default_mgr.get_recognition_param<NeuralNetworkClassifierParam>(Type::NeuralNetworkClassify);
        out_param = default_param;
        return parse_nn_classifier_param(
            input,
            std::get<NeuralNetworkClassifierParam>(out_param),
            same_type ? std::get<NeuralNetworkClassifierParam>(parent_param) : default_param);
    } break;

    case Type::NeuralNetworkDetect: {
        auto default_param = default_mgr.get_recognition_param<NeuralNetworkDetectorParam>(Type::NeuralNetworkDetect);
        out_param = default_param;
        return parse_nn_detector_param(
            input,
            std::get<NeuralNetworkDetectorParam>(out_param),
            same_type ? std::get<NeuralNetworkDetectorParam>(parent_param) : default_param);
    } break;

    case Type::OCR: {
        auto default_param = default_mgr.get_recognition_param<OCRerParam>(Type::OCR);
        out_param = default_param;
        return parse_ocrer_param(input, std::get<OCRerParam>(out_param), same_type ? std::get<OCRerParam>(parent_param) : default_param);
    } break;

    case Type::ColorMatch: {
        auto default_param = default_mgr.get_recognition_param<ColorMatcherParam>(Type::ColorMatch);
        out_param = default_param;
        return parse_color_matcher_param(
            input,
            std::get<ColorMatcherParam>(out_param),
            same_type ? std::get<ColorMatcherParam>(parent_param) : default_param);
    } break;

    case Type::Custom: {
        auto default_param = default_mgr.get_recognition_param<CustomRecognitionParam>(Type::Custom);
        out_param = default_param;
        return parse_custom_recognition_param(
            input,
            std::get<CustomRecognitionParam>(out_param),
            same_type ? std::get<CustomRecognitionParam>(parent_param) : default_param);
    } break;

    default:
        LogError << "Unknown recognition" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineResMgr::parse_template_matcher_param(
    const json::value& input,
    MAA_VISION_NS::TemplateMatcherParam& output,
    const MAA_VISION_NS::TemplateMatcherParam& default_value)
{
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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

    if (!get_and_check_value_or_array(input, "template", output.template_paths, default_value.template_paths)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "threshold", output.thresholds, default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
        return false;
    }

    if (output.thresholds.empty()) {
        output.thresholds = default_value.thresholds;
    }
    if (!output.template_paths.empty() && output.thresholds.size() != output.template_paths.size()) {
        if (output.thresholds.size() == 1) {
            double threshold = output.thresholds.front();
            output.thresholds.resize(output.template_paths.size(), threshold);
            LogDebug << "thresholds.size() != template_paths.size(), auto fill" << threshold << VAR(output.template_paths.size());
        }
        else {
            LogError << "thresholds.size() != templates.size()" << VAR(output.thresholds.size()) << VAR(output.template_paths.size());
            return false;
        }
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

    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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

    if (!get_and_check_value_or_array(input, "template", output.template_paths, default_value.template_paths)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
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
        { kDefaultDetectorFlag, default_value.detector },  { "SIFT", FeatureMatcherParam::Detector::SIFT },
        { "sift", FeatureMatcherParam::Detector::SIFT },   { "SURF", FeatureMatcherParam::Detector::SURF },
        { "surf", FeatureMatcherParam::Detector::SURF },   { "ORB", FeatureMatcherParam::Detector::ORB },
        { "orb", FeatureMatcherParam::Detector::ORB },     { "BRISK", FeatureMatcherParam::Detector::BRISK },
        { "brisk", FeatureMatcherParam::Detector::BRISK }, { "KAZE", FeatureMatcherParam::Detector::KAZE },
        { "kaze", FeatureMatcherParam::Detector::KAZE },   { "AKAZE", FeatureMatcherParam::Detector::AKAZE },
        { "akaze", FeatureMatcherParam::Detector::AKAZE },
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

    if (!get_and_check_value(input, "distance_ratio", output.distance_ratio, default_value.distance_ratio)) {
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
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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
    else {
        u8_text = u8_default_text;
    }
    std::ranges::transform(u8_text, std::back_inserter(output.expected), to_u16);

    if (!get_and_check_value(input, "threshold", output.threshold, default_value.threshold)) {
        LogError << "failed to get_and_check_value threshold" << VAR(input);
        return false;
    }

    if (auto replace_opt = input.find("replace")) {
        auto append_repalce = [&](const json::value& in) {
            auto pair = in.as<std::array<std::string, 2>>();
            output.replace.emplace_back(to_u16(pair[0]), to_u16(pair[1]));
        };

        json::value& replace = *replace_opt;

        if (replace.is<std::array<std::string, 2>>()) {
            append_repalce(replace);
        }
        else if (replace.is<std::vector<std::array<std::string, 2>>>()) {
            for (const json::value& pair : replace.as_array()) {
                append_repalce(pair);
            }
        }
        else {
            LogError << "failed to parse replace" << VAR(replace);
            return false;
        }
    }
    else {
        output.replace = default_value.replace;
    }

    return true;
}

bool PipelineResMgr::parse_custom_recognition_param(
    const json::value& input,
    MAA_VISION_NS::CustomRecognitionParam& output,
    const MAA_VISION_NS::CustomRecognitionParam& default_value)
{
    if (!get_and_check_value(input, "custom_recognition", output.name, default_value.name)) {
        LogError << "failed to get_and_check_value custom_recognition" << VAR(input);
        return false;
    }

    if (output.name.empty()) {
        LogError << "custom_recognition is empty" << VAR(input);
        return false;
    }

    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
        return false;
    }

    output.custom_param = input.get("custom_recognition_param", default_value.custom_param);

    return true;
}

bool PipelineResMgr::parse_nn_classifier_param(
    const json::value& input,
    MAA_VISION_NS::NeuralNetworkClassifierParam& output,
    const MAA_VISION_NS::NeuralNetworkClassifierParam& default_value)
{
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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
    if (!get_and_check_value(input, "model", output.model, default_value.model)) {
        LogError << "failed to get_and_check_value model" << VAR(input);
        return false;
    }
    if (!get_and_check_value_or_array(input, "labels", output.labels, default_value.labels)) {
        LogError << "failed to get_and_check_value_or_array labels" << VAR(input);
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
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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

    if (!get_and_check_value(input, "model", output.model, default_value.model)) {
        LogError << "failed to get_and_check_value model" << VAR(input);
        return false;
    }
    if (!get_and_check_value_or_array(input, "labels", output.labels, default_value.labels)) {
        LogError << "failed to get_and_check_value_or_array labels" << VAR(input);
        return false;
    }
    if (!get_and_check_value_or_array(input, "expected", output.expected, default_value.expected)) {
        LogError << "failed to get_and_check_value_or_array expected" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "threshold", output.thresholds, default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
        return false;
    }

    if (output.thresholds.empty()) {
        output.thresholds = default_value.thresholds;
    }
    if (!output.expected.empty() && output.thresholds.size() != output.expected.size()) {
        if (output.thresholds.size() == 1) {
            double threshold = output.thresholds.front();
            output.thresholds.resize(output.expected.size(), threshold);
            LogDebug << "thresholds.size() != expected.size(), auto fill" << threshold << VAR(output.expected.size());
        }
        else {
            LogError << "thresholds.size() != expected.size()" << VAR(output.thresholds.size()) << VAR(output.expected.size());
            return false;
        }
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
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
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

    if (lower.size() != upper.size()) {
        LogError << "bad size" << VAR(lower.size()) << VAR(upper.size());
        return false;
    }

    for (size_t i = 0; i != lower.size(); ++i) {
        constexpr int kMaxChannel = 4;
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

bool PipelineResMgr::parse_roi_target(const json::value& input, MAA_VISION_NS::Target& output, const MAA_VISION_NS::Target& default_value)
{
    using namespace MAA_VISION_NS;

    if (auto param_opt = input.find("roi"); !param_opt) {
        output = default_value;
    }
    else if (!parse_target_variant(*param_opt, output)) {
        LogError << "failed to parse_target_variant" << VAR(*param_opt);
        return false;
    }

    if (output.type == Target::Type::Self) {
        output.type = Target::Type::Region;
        output.param = cv::Rect();
    }

    if (auto offset_opt = input.find("roi_offset"); !offset_opt) {
        output.offset = default_value.offset;
    }
    else if (!parse_target_offset(*offset_opt, output)) {
        LogError << "failed to parse_target_offset" << VAR(*offset_opt);
        return false;
    }

    return true;
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
    std::string order_by;
    if (!get_multi_keys_and_check_value(input, { "order_by", "order" }, order_by, kDefaultOrderFlag)) {
        LogError << "failed to get_and_check_value order_by" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, MAA_VISION_NS::ResultOrderBy> kOrderMap = {
        { kDefaultOrderFlag, default_value },
        { "Horizontal", MAA_VISION_NS::ResultOrderBy::Horizontal },
        { "horizontal", MAA_VISION_NS::ResultOrderBy::Horizontal },
        { "Vertical", MAA_VISION_NS::ResultOrderBy::Vertical },
        { "vertical", MAA_VISION_NS::ResultOrderBy::Vertical },
        { "Score", MAA_VISION_NS::ResultOrderBy::Score },
        { "score", MAA_VISION_NS::ResultOrderBy::Score },
        { "Area", MAA_VISION_NS::ResultOrderBy::Area },
        { "area", MAA_VISION_NS::ResultOrderBy::Area },
        { "Length", MAA_VISION_NS::ResultOrderBy::Length },
        { "length", MAA_VISION_NS::ResultOrderBy::Length },
        { "Random", MAA_VISION_NS::ResultOrderBy::Random },
        { "random", MAA_VISION_NS::ResultOrderBy::Random },
        { "Expected", MAA_VISION_NS::ResultOrderBy::Expected },
        { "expected", MAA_VISION_NS::ResultOrderBy::Expected },
    };
    auto order_iter = kOrderMap.find(order_by);
    if (order_iter == kOrderMap.end()) {
        LogError << "order_by not found" << VAR(order_by);
        return false;
    }
    if (!valid_values.contains(order_iter->second)) {
        LogError << "current recognition not support order_by" << VAR(order_by);
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
    const Action::Type& parent_type,
    const Action::Param& parent_param,
    const DefaultPipelineMgr& default_mgr)
{
    using namespace Action;

    static const std::string kDefaultActionFlag = "Default";
    std::string act_type_name;
    if (!get_and_check_value(input, "action", act_type_name, kDefaultActionFlag)) {
        LogError << "failed to get_and_check_value action" << VAR(input);
        return false;
    }

    const std::unordered_map<std::string, Type> kActTypeMap = {
        { kDefaultActionFlag, parent_type },
        { "DoNothing", Type::DoNothing },
        { "donothing", Type::DoNothing },
        { "Click", Type::Click },
        { "click", Type::Click },
        { "Swipe", Type::Swipe },
        { "swipe", Type::Swipe },
        { "MultiSwipe", Type::MultiSwipe },
        { "multiswipe", Type::MultiSwipe },
        { "PressKey", Type::Key },
        { "presskey", Type::Key },
        { "Key", Type::Key },
        { "key", Type::Key },
        { "InputText", Type::Text },
        { "inputtext", Type::Text },
        { "Text", Type::Text },
        { "text", Type::Text },
        { "StartApp", Type::StartApp },
        { "startapp", Type::StartApp },
        { "StopApp", Type::StopApp },
        { "stopapp", Type::StopApp },
        { "Command", Type::Command },
        { "command", Type::Command },
        { "Custom", Type::Custom },
        { "custom", Type::Custom },
        { "StopTask", Type::StopTask },
        { "stoptask", Type::StopTask },
        { "Stop", Type::StopTask },
        { "stop", Type::StopTask },
    };
    auto act_type_iter = kActTypeMap.find(act_type_name);
    if (act_type_iter == kActTypeMap.cend()) {
        LogError << "act type not found" << VAR(act_type_name);
        return false;
    }
    out_type = act_type_iter->second;

    bool same_type = parent_type == out_type;
    switch (out_type) {
    case Type::DoNothing:
        return true;

    case Type::Click: {
        auto default_param = default_mgr.get_action_param<ClickParam>(Type::Click);
        out_param = default_param;
        return parse_click(input, std::get<ClickParam>(out_param), same_type ? std::get<ClickParam>(parent_param) : default_param);
    } break;

    case Type::Swipe: {
        auto default_param = default_mgr.get_action_param<SwipeParam>(Type::Swipe);
        out_param = default_param;
        return parse_swipe(input, std::get<SwipeParam>(out_param), same_type ? std::get<SwipeParam>(parent_param) : default_param);
    } break;

    case Type::MultiSwipe: {
        auto default_multi = default_mgr.get_action_param<MultiSwipeParam>(Type::MultiSwipe);
        auto default_single = default_mgr.get_action_param<SwipeParam>(Type::Swipe);
        out_param = default_multi;
        return parse_multi_swipe(
            input,
            std::get<MultiSwipeParam>(out_param),
            same_type ? std::get<MultiSwipeParam>(parent_param) : default_multi,
            default_single);
    } break;

    case Type::Key: {
        auto default_param = default_mgr.get_action_param<KeyParam>(Type::Key);
        out_param = default_param;
        return parse_press_key(input, std::get<KeyParam>(out_param), same_type ? std::get<KeyParam>(parent_param) : default_param);
    } break;

    case Type::Text: {
        auto default_param = default_mgr.get_action_param<TextParam>(Type::Text);
        out_param = default_param;
        return parse_input_text(input, std::get<TextParam>(out_param), same_type ? std::get<TextParam>(parent_param) : default_param);
    } break;

    case Type::StartApp: {
        auto default_param = default_mgr.get_action_param<AppParam>(Type::StartApp);
        out_param = default_param;
        return parse_app_info(input, std::get<AppParam>(out_param), same_type ? std::get<AppParam>(parent_param) : default_param);
    } break;

    case Type::StopApp: {
        auto default_param = default_mgr.get_action_param<AppParam>(Type::StopApp);
        out_param = default_param;
        return parse_app_info(input, std::get<AppParam>(out_param), same_type ? std::get<AppParam>(parent_param) : default_param);
    } break;

    case Type::Command: {
        auto default_param = default_mgr.get_action_param<CommandParam>(Type::Command);
        out_param = default_param;
        return parse_command_param(
            input,
            std::get<CommandParam>(out_param),
            same_type ? std::get<CommandParam>(parent_param) : default_param);
    } break;

    case Type::Custom: {
        auto default_param = default_mgr.get_action_param<CustomParam>(Type::Custom);
        out_param = default_param;
        return parse_custom_action_param(
            input,
            std::get<CustomParam>(out_param),
            same_type ? std::get<CustomParam>(parent_param) : default_param);
    } break;

    case Type::StopTask:
        out_param = {};
        return true;

    default:
        LogError << "unknown act type" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineResMgr::parse_click(const json::value& input, Action::ClickParam& output, const Action::ClickParam& default_value)
{
    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_swipe(const json::value& input, Action::SwipeParam& output, const Action::SwipeParam& default_value)
{
    if (!parse_action_target(input, "begin", output.begin, default_value.begin)) {
        LogError << "failed to parse_action_target begin" << VAR(input);
        return false;
    }

    if (!parse_action_target(input, "end", output.end, default_value.end)) {
        LogError << "failed to parse_action_target end" << VAR(input);
        return false;
    }
    // if (output.begin.type == Action::Target::Type::Self
    //     && output.end.type == Action::Target::Type::Self) {
    //     LogError << "not set swipe begin or end";
    //     return false;
    // }

    if (!get_and_check_value(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    // only for multi swipe
    if (!get_and_check_value(input, "starting", output.starting, default_value.starting)) {
        LogError << "failed to get_and_check_value starting" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_multi_swipe(
    const json::value& input,
    Action::MultiSwipeParam& output,
    const Action::MultiSwipeParam& default_multi,
    const Action::SwipeParam& default_single)
{
    auto swipes_opt = input.find("swipes");
    if (!swipes_opt) {
        output = default_multi;
        return true;
    }

    if (!swipes_opt->is_array()) {
        LogError << "failed to parse swipes, is not array" << VAR(input);
        return false;
    }

    output.swipes.clear();
    for (const json::value& swipe : swipes_opt->as_array()) {
        Action::SwipeParam res;
        if (!parse_swipe(swipe, res, default_single)) {
            return false;
        }

        output.swipes.emplace_back(std::move(res));
    }

    return true;
}

bool PipelineResMgr::parse_press_key(const json::value& input, Action::KeyParam& output, const Action::KeyParam& default_value)
{
    // TODO: https://github.com/MaaXYZ/MaaFramework/issues/24#issuecomment-1666533842
    if (!get_and_check_value_or_array(input, "key", output.keys, default_value.keys)) {
        LogError << "failed to get_and_check_value_or_array key" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_input_text(const json::value& input, Action::TextParam& output, const Action::TextParam& default_value)
{
    if (!get_and_check_value(input, "input_text", output.text, default_value.text)) {
        LogError << "failed to get_and_check_value text" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_app_info(const json::value& input, Action::AppParam& output, const Action::AppParam& default_value)
{
    if (!get_and_check_value(input, "package", output.package, default_value.package)) {
        LogError << "failed to get_and_check_value activity" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_command_param(const json::value& input, Action::CommandParam& output, const Action::CommandParam& default_value)
{
    if (!get_and_check_value(input, "exec", output.exec, default_value.exec)) {
        LogError << "failed to get_and_check_value exec" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "args", output.args, default_value.args)) {
        LogError << "failed to get_and_check_value args" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "detach", output.detach, default_value.detach)) {
        LogError << "failed to get_and_check_value detach" << VAR(input);
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

    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    if (output.name.empty()) {
        LogError << "custom_action is empty" << VAR(input);
        return false;
    }

    output.custom_param = input.get("custom_action_param", default_value.custom_param);

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
        output = default_value;
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

        auto rate_limit = default_value.rate_limit.count();
        if (!get_and_check_value(field, "rate_limit", rate_limit, rate_limit)) {
            LogError << "failed to parse_wait_freezes_param rate_limit" << VAR(field);
            return false;
        }
        output.rate_limit = std::chrono::milliseconds(rate_limit);

        auto timeout = default_value.timeout.count();
        if (!get_and_check_value(field, "timeout", timeout, timeout)) {
            LogError << "failed to parse_wait_freezes_param timeout" << VAR(field);
            return false;
        }
        output.timeout = std::chrono::milliseconds(timeout);

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

bool PipelineResMgr::parse_target_variant(const json::value& input_target, Action::Target& output)
{
    using namespace Action;

    if (input_target.is_boolean() && input_target.as_boolean()) {
        output.type = Target::Type::Self;
    }
    else if (input_target.is_string()) {
        output.type = Target::Type::PreTask;
        output.param = input_target.as_string();
    }
    else if (input_target.is_array()) {
        output.type = Target::Type::Region;
        cv::Rect rect {};
        if (!parse_rect(input_target, rect)) {
            LogError << "Target::Type::Region failed to parse_rect" << VAR(input_target);
            return false;
        }
        output.param = rect;
    }
    else {
        LogError << "param type error" << VAR(input_target);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_target_offset(const json::value& input_target, Action::Target& output)
{
    if (input_target.is_array()) {
        if (!parse_rect(input_target, output.offset)) {
            LogError << "failed to parse_rect" << VAR(input_target);
            return false;
        }
    }
    else {
        LogError << "offset type error" << VAR(input_target);
        return false;
    }

    return true;
}

bool PipelineResMgr::parse_action_target(
    const json::value& input,
    const std::string& key,
    Action::Target& output,
    const Action::Target& default_value)
{
    if (auto param_opt = input.find(key); !param_opt) {
        output = default_value;
    }
    else if (!parse_target_variant(*param_opt, output)) {
        LogError << "failed to parse_target_variant" << VAR(*param_opt);
        return false;
    }

    if (auto offset_opt = input.find(key + "_offset"); !offset_opt) {
        output.offset = default_value.offset;
    }
    else if (!parse_target_offset(*offset_opt, output)) {
        LogError << "failed to parse_target_offset" << VAR(*offset_opt);
        return false;
    }

    return true;
}

MAA_RES_NS_END
