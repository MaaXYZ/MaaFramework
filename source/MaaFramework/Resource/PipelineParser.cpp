#include "PipelineParser.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"
#include "PipelineTypesV2.h"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

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
            continue;
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
    output.clear();

    auto opt = input.find(key);
    if (!opt) {
        output = default_value;
        return true;
    }

    if (opt->is_array()) {
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

template <typename OutT>
bool get_multi_keys_and_check_value_or_array(
    const json::value& input,
    const std::vector<std::string>& keys,
    std::vector<OutT>& output,
    const std::vector<OutT>& default_value)
{
    output.clear();

    for (const auto& k : keys) {
        auto opt = input.find(k);
        if (!opt) {
            continue;
        }
        else if (opt->is_array()) {
            for (const auto& item : opt->as_array()) {
                if (!item.is<OutT>()) {
                    LogError << "type error" << VAR(keys) << VAR(input);
                    return false;
                }
                output.emplace_back(item.as<OutT>());
            }
            return true;
        }
        else if (opt->is<OutT>()) {
            output = { opt->as<OutT>() };
            return true;
        }
        else {
            LogError << "type error" << VAR(keys) << VAR(input);
            return false;
        }
    }

    output = default_value;
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

bool PipelineParser::parse_node(
    const std::string& name,
    const json::value& input,
    PipelineData& output,
    const PipelineData& default_value,
    const DefaultPipelineMgr& default_mgr)
{
    LogDebug << VAR(name);

    if (!input.is_object()) {
        LogError << "input is not object" << VAR(input);
        return false;
    }

    PipelineData data;
    data.name = name;

    // 检查已废弃的字段
    if (input.exists("is_sub")) {
        LogError << "is_sub is deprecated since v5.1, in version 5.1, use `[JumpBack]` instead, please refer to the document for details."
                 << VAR(name);
        LogError << "We provide a migration script (https://github.com/MaaXYZ/MaaFramework/blob/main/tools/migrate_pipeline_v5.py) to "
                    "help you automatically convert your pipeline to the new style without loss of quality.";

        LogError << "is_sub 已在 5.1 版本中废弃，推荐使用 `[JumpBack]` 替代，详情请参考文档" << VAR(name);
        LogError << "我们提供了一个迁移脚本 (https://github.com/MaaXYZ/MaaFramework/blob/main/tools/migrate_pipeline_v5.py)， "
                    "可帮助您无损地将 Pipeline 自动转换为新风格~";
        return false;
    }
    if (input.exists("interrupt")) {
        LogError
            << "interrupt is deprecated since v5.1, in version 5.1, use `[JumpBack]` instead, please refer to the document for details."
            << VAR(name);
        LogError << "We provide a migration script (https://github.com/MaaXYZ/MaaFramework/blob/main/tools/migrate_pipeline_v5.py) to "
                    "help you automatically convert your pipeline to the new style without loss of quality.";

        LogError << "interrupt 已在 5.1 版本中废弃，推荐使用 `[JumpBack]` 替代，详情请参考文档" << VAR(name);
        LogError << "我们提供了一个迁移脚本 (https://github.com/MaaXYZ/MaaFramework/blob/main/tools/migrate_pipeline_v5.py)， "
                    "可帮助您无损地将 Pipeline 自动转换为新风格~";
        return false;
    }

    if (!get_and_check_value(input, "inverse", data.inverse, default_value.inverse)) {
        LogError << "failed to get_and_check_value inverse" << VAR(input);
        return false;
    }

    if (!get_multi_keys_and_check_value(input, { "enable", "enabled" }, data.enabled, default_value.enabled)) {
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

    if (!parse_next(input, "next", data.next, default_value.next)) {
        LogError << "failed to parse_next" << VAR(input);
        return false;
    }

    if (!parse_next(input, "on_error", data.on_error, default_value.on_error)) {
        LogError << "failed to parse on_error" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "anchor", data.anchor, default_value.anchor)) {
        LogError << "failed to get_and_check_value_or_array anchor" << VAR(input);
        return false;
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

    if (!get_and_check_value(input, "repeat", data.repeat, default_value.repeat)) {
        LogError << "failed to get_and_check_value repeat" << VAR(input);
        return false;
    }

    auto repeat_delay = default_value.repeat_delay.count();
    if (!get_and_check_value(input, "repeat_delay", repeat_delay, repeat_delay)) {
        LogError << "failed to get_and_check_value repeat_delay" << VAR(input);
        return false;
    }
    data.repeat_delay = std::chrono::milliseconds(repeat_delay);

    if (!parse_wait_freezes_param(input, "repeat_wait_freezes", data.repeat_wait_freezes, default_value.repeat_wait_freezes)) {
        LogError << "failed to repeat_wait_freezes" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "max_hit", data.max_hit, default_value.max_hit)) {
        LogError << "failed to get_and_check_value max_hit" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "focus", data.focus, default_value.focus)) {
        LogError << "failed to get_and_check_value focus" << VAR(input);
        return false;
    }

    data.attach = default_value.attach;
    if (auto attach_opt = input.find<json::object>("attach")) {
        data.attach |= *std::move(attach_opt);
    }

    output = std::move(data);

    return true;
}

bool PipelineParser::parse_recognition(
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

    json::value param_input = input;

    if (auto reco_opt = input.find("recognition"); reco_opt && reco_opt->is_object()) {
        param_input = reco_opt->get("param", json::object());
        reco_type_name = reco_opt->get("type", kDefaultRecognitionFlag);
    }
    else if (!get_and_check_value(input, "recognition", reco_type_name, kDefaultRecognitionFlag)) {
        LogError << "failed to get_and_check_value recognition" << VAR(input);
        return false;
    }

    auto rec_type_map = kTypeMap;
    rec_type_map.insert_or_assign(kDefaultRecognitionFlag, parent_type);

    auto reco_type_iter = rec_type_map.find(reco_type_name);
    if (reco_type_iter == rec_type_map.end()) {
        LogError << "rec type not found" << VAR(reco_type_name);
        return false;
    }
    out_type = reco_type_iter->second;

    bool same_type = parent_type == out_type;
    switch (out_type) {
    case Type::DirectHit: {
        auto default_param = default_mgr.get_recognition_param<DirectHitParam>(Type::DirectHit);
        out_param = default_param;
        return parse_direct_hit_param(
            param_input,
            std::get<DirectHitParam>(out_param),
            same_type ? std::get<DirectHitParam>(parent_param) : default_param);
    } break;

    case Type::TemplateMatch: {
        auto default_param = default_mgr.get_recognition_param<TemplateMatcherParam>(Type::TemplateMatch);
        out_param = default_param;
        return parse_template_matcher_param(
            param_input,
            std::get<TemplateMatcherParam>(out_param),
            same_type ? std::get<TemplateMatcherParam>(parent_param) : default_param);
    } break;

    case Type::FeatureMatch: {
        auto default_param = default_mgr.get_recognition_param<FeatureMatcherParam>(Type::FeatureMatch);
        out_param = default_param;
        return parse_feature_matcher_param(
            param_input,
            std::get<FeatureMatcherParam>(out_param),
            same_type ? std::get<FeatureMatcherParam>(parent_param) : default_param);
    } break;

    case Type::NeuralNetworkClassify: {
        auto default_param = default_mgr.get_recognition_param<NeuralNetworkClassifierParam>(Type::NeuralNetworkClassify);
        out_param = default_param;
        return parse_nn_classifier_param(
            param_input,
            std::get<NeuralNetworkClassifierParam>(out_param),
            same_type ? std::get<NeuralNetworkClassifierParam>(parent_param) : default_param);
    } break;

    case Type::NeuralNetworkDetect: {
        auto default_param = default_mgr.get_recognition_param<NeuralNetworkDetectorParam>(Type::NeuralNetworkDetect);
        out_param = default_param;
        return parse_nn_detector_param(
            param_input,
            std::get<NeuralNetworkDetectorParam>(out_param),
            same_type ? std::get<NeuralNetworkDetectorParam>(parent_param) : default_param);
    } break;

    case Type::OCR: {
        auto default_param = default_mgr.get_recognition_param<OCRerParam>(Type::OCR);
        out_param = default_param;
        return parse_ocrer_param(
            param_input,
            std::get<OCRerParam>(out_param),
            same_type ? std::get<OCRerParam>(parent_param) : default_param);
    } break;

    case Type::ColorMatch: {
        auto default_param = default_mgr.get_recognition_param<ColorMatcherParam>(Type::ColorMatch);
        out_param = default_param;
        return parse_color_matcher_param(
            param_input,
            std::get<ColorMatcherParam>(out_param),
            same_type ? std::get<ColorMatcherParam>(parent_param) : default_param);
    } break;

    case Type::And: {
        auto param = std::make_shared<AndParam>();
        out_param = param;
        return parse_and_param(param_input, param, default_mgr);
    } break;

    case Type::Or: {
        auto param = std::make_shared<OrParam>();
        out_param = param;
        return parse_or_param(param_input, param, default_mgr);
    } break;

    case Type::Custom: {
        auto default_param = default_mgr.get_recognition_param<CustomRecognitionParam>(Type::Custom);
        out_param = default_param;
        return parse_custom_recognition_param(
            param_input,
            std::get<CustomRecognitionParam>(out_param),
            same_type ? std::get<CustomRecognitionParam>(parent_param) : default_param);
    } break;

    default:
        LogError << "Unknown recognition" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineParser::parse_direct_hit_param(
    const json::value& input,
    MAA_VISION_NS::DirectHitParam& output,
    const MAA_VISION_NS::DirectHitParam& default_value)
{
    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_template_matcher_param(
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

    if (!get_and_check_value_or_array(input, "template", output.template_, default_value.template_)) {
        LogError << "failed to get_and_check_value_or_array templates" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "threshold", output.thresholds, default_value.thresholds)) {
        LogError << "failed to get_and_check_value_or_array threshold" << VAR(input);
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

bool PipelineParser::parse_feature_matcher_param(
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

    if (!get_and_check_value_or_array(input, "template", output.template_, default_value.template_)) {
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

    if (!get_and_check_value(input, "ratio", output.ratio, default_value.ratio)) {
        LogError << "failed to get_and_check_value ratio" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "count", output.count, default_value.count)) {
        LogError << "failed to get_and_check_value count" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_ocrer_param(
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
                MAA_VISION_NS::ResultOrderBy::Expected,
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

    output.replace.clear();

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

bool PipelineParser::parse_custom_recognition_param(
    const json::value& input,
    MAA_VISION_NS::CustomRecognitionParam& output,
    const MAA_VISION_NS::CustomRecognitionParam& default_value)
{
    if (!get_and_check_value(input, "custom_recognition", output.name, default_value.name)) {
        LogError << "failed to get_and_check_value custom_recognition" << VAR(input);
        return false;
    }

    if (!parse_roi_target(input, output.roi_target, default_value.roi_target)) {
        LogError << "failed to parse_roi_target" << VAR(input);
        return false;
    }

    output.custom_param = input.get("custom_recognition_param", default_value.custom_param);

    return true;
}

bool PipelineParser::parse_nn_classifier_param(
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
                MAA_VISION_NS::ResultOrderBy::Expected,
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

bool PipelineParser::parse_nn_detector_param(
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
                MAA_VISION_NS::ResultOrderBy::Expected,
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

bool PipelineParser::parse_color_matcher_param(
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

    output.range.clear();

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

bool PipelineParser::parse_roi_target(const json::value& input, MAA_VISION_NS::Target& output, const MAA_VISION_NS::Target& default_value)
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
    else if (!parse_rect(*offset_opt, output.offset)) {
        LogError << "failed to parse_target_offset" << VAR(*offset_opt);
        return false;
    }

    return true;
}

bool PipelineParser::parse_order_of_result(
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

bool PipelineParser::parse_action(
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

    json::value param_input = input;

    if (auto reco_opt = input.find("action"); reco_opt && reco_opt->is_object()) {
        param_input = reco_opt->get("param", json::object());
        act_type_name = reco_opt->get("type", kDefaultActionFlag);
    }
    else if (!get_and_check_value(param_input, "action", act_type_name, kDefaultActionFlag)) {
        LogError << "failed to get_and_check_value action" << VAR(input);
        return false;
    }

    auto act_type_map = kTypeMap;
    act_type_map.insert_or_assign(kDefaultActionFlag, parent_type);

    auto act_type_iter = act_type_map.find(act_type_name);
    if (act_type_iter == act_type_map.cend()) {
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
        return parse_click(param_input, std::get<ClickParam>(out_param), same_type ? std::get<ClickParam>(parent_param) : default_param);
    } break;

    case Type::LongPress: {
        auto default_param = default_mgr.get_action_param<LongPressParam>(Type::LongPress);
        out_param = default_param;
        return parse_long_press(
            param_input,
            std::get<LongPressParam>(out_param),
            same_type ? std::get<LongPressParam>(parent_param) : default_param);
    } break;

    case Type::Swipe: {
        auto default_param = default_mgr.get_action_param<SwipeParam>(Type::Swipe);
        out_param = default_param;
        return parse_swipe(param_input, std::get<SwipeParam>(out_param), same_type ? std::get<SwipeParam>(parent_param) : default_param);
    } break;

    case Type::MultiSwipe: {
        auto default_multi = default_mgr.get_action_param<MultiSwipeParam>(Type::MultiSwipe);
        auto default_single = default_mgr.get_action_param<SwipeParam>(Type::Swipe);
        out_param = default_multi;
        return parse_multi_swipe(
            param_input,
            std::get<MultiSwipeParam>(out_param),
            same_type ? std::get<MultiSwipeParam>(parent_param) : default_multi,
            default_single);
    } break;

    case Type::TouchDown: {
        auto default_param = default_mgr.get_action_param<TouchParam>(Type::TouchDown);
        out_param = default_param;
        return parse_touch(param_input, std::get<TouchParam>(out_param), same_type ? std::get<TouchParam>(parent_param) : default_param);
    } break;

    case Type::TouchMove: {
        auto default_param = default_mgr.get_action_param<TouchParam>(Type::TouchMove);
        out_param = default_param;
        return parse_touch(param_input, std::get<TouchParam>(out_param), same_type ? std::get<TouchParam>(parent_param) : default_param);
    } break;

    case Type::TouchUp: {
        auto default_param = default_mgr.get_action_param<TouchUpParam>(Type::TouchUp);
        out_param = default_param;
        return parse_touch_up(
            param_input,
            std::get<TouchUpParam>(out_param),
            same_type ? std::get<TouchUpParam>(parent_param) : default_param);
    } break;

    case Type::ClickKey: {
        auto default_param = default_mgr.get_action_param<ClickKeyParam>(Type::ClickKey);
        out_param = default_param;
        return parse_click_key(
            param_input,
            std::get<ClickKeyParam>(out_param),
            same_type ? std::get<ClickKeyParam>(parent_param) : default_param);
    } break;

    case Type::KeyDown: {
        auto default_param = default_mgr.get_action_param<KeyParam>(Type::KeyDown);
        out_param = default_param;
        return parse_key_param(param_input, std::get<KeyParam>(out_param), same_type ? std::get<KeyParam>(parent_param) : default_param);
    } break;

    case Type::KeyUp: {
        auto default_param = default_mgr.get_action_param<KeyParam>(Type::KeyUp);
        out_param = default_param;
        return parse_key_param(param_input, std::get<KeyParam>(out_param), same_type ? std::get<KeyParam>(parent_param) : default_param);
    } break;

    case Type::LongPressKey: {
        auto default_param = default_mgr.get_action_param<LongPressKeyParam>(Type::LongPressKey);
        out_param = default_param;
        return parse_long_press_key(
            param_input,
            std::get<LongPressKeyParam>(out_param),
            same_type ? std::get<LongPressKeyParam>(parent_param) : default_param);
    } break;

    case Type::InputText: {
        auto default_param = default_mgr.get_action_param<InputTextParam>(Type::InputText);
        out_param = default_param;
        return parse_input_text(
            param_input,
            std::get<InputTextParam>(out_param),
            same_type ? std::get<InputTextParam>(parent_param) : default_param);
    } break;

    case Type::StartApp: {
        auto default_param = default_mgr.get_action_param<AppParam>(Type::StartApp);
        out_param = default_param;
        return parse_app_info(param_input, std::get<AppParam>(out_param), same_type ? std::get<AppParam>(parent_param) : default_param);
    } break;

    case Type::StopApp: {
        auto default_param = default_mgr.get_action_param<AppParam>(Type::StopApp);
        out_param = default_param;
        return parse_app_info(param_input, std::get<AppParam>(out_param), same_type ? std::get<AppParam>(parent_param) : default_param);
    } break;

    case Type::Scroll: {
        auto default_param = default_mgr.get_action_param<ScrollParam>(Type::Scroll);
        out_param = default_param;
        return parse_scroll(param_input, std::get<ScrollParam>(out_param), same_type ? std::get<ScrollParam>(parent_param) : default_param);
    } break;

    case Type::StopTask:
        out_param = {};
        return true;

    case Type::Command: {
        auto default_param = default_mgr.get_action_param<CommandParam>(Type::Command);
        out_param = default_param;
        return parse_command_param(
            param_input,
            std::get<CommandParam>(out_param),
            same_type ? std::get<CommandParam>(parent_param) : default_param);
    } break;

    case Type::Shell: {
        auto default_param = default_mgr.get_action_param<ShellParam>(Type::Shell);
        out_param = default_param;
        return parse_shell(param_input, std::get<ShellParam>(out_param), same_type ? std::get<ShellParam>(parent_param) : default_param);
    } break;

    case Type::Custom: {
        auto default_param = default_mgr.get_action_param<CustomParam>(Type::Custom);
        out_param = default_param;
        return parse_custom_action_param(
            param_input,
            std::get<CustomParam>(out_param),
            same_type ? std::get<CustomParam>(parent_param) : default_param);
    } break;

    default:
        LogError << "unknown act type" << VAR(static_cast<int>(out_type));
        return false;
    }

    return false;
}

bool PipelineParser::parse_click(const json::value& input, Action::ClickParam& output, const Action::ClickParam& default_value)
{
    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "contact", output.contact, default_value.contact)) {
        LogError << "failed to get_and_check_value contact" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_long_press(const json::value& input, Action::LongPressParam& output, const Action::LongPressParam& default_value)
{
    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "contact", output.contact, default_value.contact)) {
        LogError << "failed to get_and_check_value contact" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_swipe(const json::value& input, Action::SwipeParam& output, const Action::SwipeParam& default_value)
{
    if (!parse_action_target(input, "begin", output.begin, default_value.begin)) {
        LogError << "failed to parse_action_target begin" << VAR(input);
        return false;
    }

    if (!parse_action_target_obj_or_list(input, "end", output.end, default_value.end)) {
        LogError << "failed to parse_action_target end" << VAR(input);
        return false;
    }

    if (!parse_action_target_offset_or_list(input, "end_offset", output.end_offset, default_value.end_offset)) {
        LogError << "failed to parse_action_target end" << VAR(input);
        return false;
    }
    // if (output.begin.type == Action::Target::Type::Self
    //     && output.end.type == Action::Target::Type::Self) {
    //     LogError << "not set swipe begin or end";
    //     return false;
    // }

    if (!get_and_check_value_or_array(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    if (!get_and_check_value_or_array(input, "end_hold", output.end_hold, default_value.end_hold)) {
        LogError << "failed to get_and_check_value end_hold" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "only_hover", output.only_hover, default_value.only_hover)) {
        LogError << "failed to get_and_check_value only_hover" << VAR(input);
        return false;
    }

    // only for multi swipe
    if (!get_and_check_value(input, "starting", output.starting, default_value.starting)) {
        LogError << "failed to get_and_check_value starting" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "contact", output.contact, default_value.contact)) {
        LogError << "failed to get_and_check_value contact" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_multi_swipe(
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

bool PipelineParser::parse_key_param(const json::value& input, Action::KeyParam& output, const Action::KeyParam& default_value)
{
    int key = default_value.key;
    if (!get_multi_keys_and_check_value(input, { "key", "key_code" }, key, default_value.key)) {
        LogError << "failed to get_multi_keys_and_check_value key" << VAR(input);
        return false;
    }

    output.key = key;
    return true;
}

bool PipelineParser::parse_touch(const json::value& input, Action::TouchParam& output, const Action::TouchParam& default_value)
{
    if (!get_and_check_value(input, "contact", output.contact, default_value.contact)) {
        LogError << "failed to get_and_check_value contact" << VAR(input);
        return false;
    }

    if (!parse_action_target(input, "target", output.target, default_value.target)) {
        LogError << "failed to parse_action_target target" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "pressure", output.pressure, default_value.pressure)) {
        LogError << "failed to get_and_check_value pressure" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_touch_up(const json::value& input, Action::TouchUpParam& output, const Action::TouchUpParam& default_value)
{
    if (!get_and_check_value(input, "contact", output.contact, default_value.contact)) {
        LogError << "failed to get_and_check_value contact" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_click_key(const json::value& input, Action::ClickKeyParam& output, const Action::ClickKeyParam& default_value)
{
    // TODO: https://github.com/MaaXYZ/MaaFramework/issues/24#issuecomment-1666533842
    if (!get_and_check_value_or_array(input, "key", output.keys, default_value.keys)) {
        LogError << "failed to get_and_check_value_or_array key" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_long_press_key(
    const json::value& input,
    Action::LongPressKeyParam& output,
    const Action::LongPressKeyParam& default_value)
{
    // TODO: https://github.com/MaaXYZ/MaaFramework/issues/24#issuecomment-1666533842
    if (!get_and_check_value_or_array(input, "key", output.keys, default_value.keys)) {
        LogError << "failed to get_and_check_value_or_array key" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "duration", output.duration, default_value.duration)) {
        LogError << "failed to get_and_check_value duration" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_input_text(const json::value& input, Action::InputTextParam& output, const Action::InputTextParam& default_value)
{
    if (!get_and_check_value(input, "input_text", output.text, default_value.text)) {
        LogError << "failed to get_and_check_value text" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_app_info(const json::value& input, Action::AppParam& output, const Action::AppParam& default_value)
{
    if (!get_and_check_value(input, "package", output.package, default_value.package)) {
        LogError << "failed to get_and_check_value activity" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_scroll(const json::value& input, Action::ScrollParam& output, const Action::ScrollParam& default_value)
{
    if (!get_and_check_value(input, "dx", output.dx, default_value.dx)) {
        LogError << "failed to get_and_check_value dx" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "dy", output.dy, default_value.dy)) {
        LogError << "failed to get_and_check_value dy" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_shell(const json::value& input, Action::ShellParam& output, const Action::ShellParam& default_value)
{
    if (!get_and_check_value(input, "cmd", output.cmd, default_value.cmd)) {
        LogError << "failed to get_and_check_value cmd" << VAR(input);
        return false;
    }

    if (!get_and_check_value(input, "timeout", output.timeout, default_value.timeout)) {
        LogError << "failed to get_and_check_value timeout" << VAR(input);
        return false;
    }

    return true;
}

bool PipelineParser::parse_command_param(const json::value& input, Action::CommandParam& output, const Action::CommandParam& default_value)
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

bool PipelineParser::parse_custom_action_param(
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

    output.custom_param = input.get("custom_action_param", default_value.custom_param);

    return true;
}

bool PipelineParser::parse_wait_freezes_param(
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

bool PipelineParser::parse_next(
    const json::value& input,
    const std::string& key,
    std::vector<NodeAttr>& output,
    const std::vector<NodeAttr>& default_next)
{
    auto next_opt = input.find(key);
    if (!next_opt) {
        output = default_next;
        return true;
    }

    return parse_next(*next_opt, output);
}

bool PipelineParser::parse_next(const json::value& input, std::vector<NodeAttr>& output)
{
    output.clear();

    if (input.is_array()) {
        for (const auto& val : input.as_array()) {
            NodeAttr res;
            if (!parse_node_in_next(val, res)) {
                LogError << "failed to parse_node_in_next" << VAR(val);
                return false;
            }
            output.emplace_back(std::move(res));
        }
    }
    else {
        NodeAttr res;
        if (!parse_node_in_next(input, res)) {
            LogError << "failed to parse_node_in_next" << VAR(input);
            return false;
        }
        output.emplace_back(std::move(res));
    }

    return true;
}

bool PipelineParser::parse_rect(const json::value& input_rect, cv::Rect& output)
{
    if (!input_rect.is_array()) {
        LogError << "rect is not array" << VAR(input_rect);
        return false;
    }

    auto& rect_array = input_rect.as_array();
    if (rect_array.empty()) {
        LogError << "rect size invalid" << VAR(rect_array.size());
        return false;
    }

    std::vector<int> rect_vec;
    for (const auto& r : rect_array) {
        if (!r.is_number()) {
            LogError << "type error" << VAR(r) << "is not integer";
            return false;
        }
        rect_vec.emplace_back(r.as_integer());
    }
    if (rect_vec.size() == 2) {
        output = cv::Rect(rect_vec[0], rect_vec[1], 0, 0);
    }
    else if (rect_vec.size() == 4) {
        output = cv::Rect(rect_vec[0], rect_vec[1], rect_vec[2], rect_vec[3]);
    }
    else {
        LogError << "rect size unsupported" << VAR(rect_vec.size());
        return false;
    }
    return true;
}

template <typename TargetType>
bool PipelineParser::parse_target_variant(const json::value& input_target, TargetType& output)
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

bool PipelineParser::parse_action_target(
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
    else if (!parse_rect(*offset_opt, output.offset)) {
        LogError << "failed to parse_target_offset" << VAR(*offset_opt);
        return false;
    }

    return true;
}

bool PipelineParser::parse_action_target_obj_or_list(
    const json::value& input,
    const std::string& key,
    std::vector<Action::TargetObj>& output,
    const std::vector<Action::TargetObj>& default_value)
{
    output.clear();

    if (auto param_opt = input.find(key); !param_opt) {
        output = default_value;
    }
    else if (param_opt->is_array() && !param_opt->is<std::array<int, 4>>() && !param_opt->is<std::array<int, 2>>()) {
        for (const auto& val : param_opt->as_array()) {
            Action::TargetObj res;
            if (!parse_target_variant(val, res)) {
                LogError << "failed to parse_target_variant" << VAR(val);
                return false;
            }
            output.emplace_back(std::move(res));
        }
    }
    else {
        Action::TargetObj res;
        if (!parse_target_variant(*param_opt, res)) {
            LogError << "failed to parse_target_variant" << VAR(*param_opt);
            return false;
        }
        output.emplace_back(std::move(res));
    }

    return true;
}

bool PipelineParser::parse_action_target_offset_or_list(
    const json::value& input,
    const std::string& key,
    std::vector<cv::Rect>& output,
    const std::vector<cv::Rect>& default_value)
{
    output.clear();

    if (auto offset_opt = input.find(key); !offset_opt) {
        output = default_value;
    }
    else if (offset_opt->is_array() && !offset_opt->is<std::array<int, 4>>() && !offset_opt->is<std::array<int, 2>>()) {
        for (const auto& val : offset_opt->as_array()) {
            cv::Rect res;
            if (!parse_rect(val, res)) {
                LogError << "failed to parse_target_offset" << VAR(val);
                return false;
            }
            output.emplace_back(std::move(res));
        }
    }
    else {
        cv::Rect res;
        if (!parse_rect(*offset_opt, res)) {
            LogError << "failed to parse_target_offset" << VAR(input);
            return false;
        }
        output.emplace_back(std::move(res));
    }

    return true;
}

bool PipelineParser::parse_node_in_next(const json::value& input, NodeAttr& output)
{
    if (input.is_string()) {
        return parse_node_string_in_next(input.as_string(), output);
    }
    else if (input.is<NodeAttr>()) {
        output = input.as<NodeAttr>();
        return true;
    }
    else {
        LogError << "next node type error" << VAR(input);
        return false;
    }
}

bool PipelineParser::parse_node_string_in_next(const std::string& raw, NodeAttr& output)
{
    std::string remaining = raw;

    // 属性必须放在节点名前面，格式: [Attr1][Attr2]NodeName
    while (remaining.starts_with('[')) {
        auto end_pos = remaining.find(']');
        if (end_pos == std::string::npos) {
            LogError << "Invalid node attribute format, missing ']'" << VAR(raw);
            return false;
        }

        std::string attr = remaining.substr(0, end_pos + 1);

        if (attr == PipelineData::kNodeAttr_JumpBack) {
            output.jump_back = true;
        }
        else if (attr == PipelineData::kNodeAttr_Anchor) {
            output.anchor = true;
        }
        else {
            LogWarn << "Unrecognized node attribute" << VAR(attr) << VAR(raw);
        }

        remaining = remaining.substr(end_pos + 1);
    }

    if (remaining.empty()) {
        LogError << "Invalid node format, missing node name or anchor name" << VAR(raw);
        return false;
    }

    output.name = remaining;
    return true;
}

bool PipelineParser::parse_and_param(
    const json::value& input,
    std::shared_ptr<Recognition::AndParam>& output,
    const DefaultPipelineMgr& default_mgr)
{
    if (!output) {
        output = std::make_shared<Recognition::AndParam>();
    }
    output->all_of.clear();

    auto all_opt = input.find("all_of");
    if (!all_opt || !all_opt->is_array() || all_opt->as_array().empty()) {
        LogError << "And recognition requires non-empty 'all_of' array field" << VAR(input);
        return false;
    }

    for (const auto& sub_input : all_opt->as_array()) {
        Recognition::SubRecognition sub_reco;
        if (!parse_sub_recognition(sub_input, sub_reco, default_mgr)) {
            LogError << "failed to parse sub recognition in 'all_of'" << VAR(sub_input);
            return false;
        }
        output->all_of.emplace_back(std::move(sub_reco));
    }

    if (!get_and_check_value(input, "box_index", output->box_index, 0)) {
        LogError << "failed to get_and_check_value box_index" << VAR(input);
        return false;
    }

    if (output->box_index < 0 || output->box_index >= static_cast<int>(output->all_of.size())) {
        LogError << "box_index out of range" << VAR(output->box_index) << VAR(output->all_of.size());
        return false;
    }

    return true;
}

bool PipelineParser::parse_or_param(
    const json::value& input,
    std::shared_ptr<Recognition::OrParam>& output,
    const DefaultPipelineMgr& default_mgr)
{
    if (!output) {
        output = std::make_shared<Recognition::OrParam>();
    }
    output->any_of.clear();

    auto any_opt = input.find("any_of");
    if (!any_opt || !any_opt->is_array() || any_opt->as_array().empty()) {
        LogError << "Or recognition requires non-empty 'any_of' array field" << VAR(input);
        return false;
    }

    for (const auto& sub_input : any_opt->as_array()) {
        Recognition::SubRecognition sub_reco;
        if (!parse_sub_recognition(sub_input, sub_reco, default_mgr)) {
            LogError << "failed to parse sub recognition in 'any_of'" << VAR(sub_input);
            return false;
        }
        output->any_of.emplace_back(std::move(sub_reco));
    }

    return true;
}

bool PipelineParser::parse_sub_recognition(
    const json::value& input,
    Recognition::SubRecognition& output,
    const DefaultPipelineMgr& default_mgr)
{
    using namespace Recognition;
    using namespace MAA_VISION_NS;

    // Default parent type and param for sub-recognition
    Type parent_type = Type::DirectHit;
    Param parent_param = DirectHitParam {};

    if (!parse_recognition(input, output.type, output.param, parent_type, parent_param, default_mgr)) {
        return false;
    }

    if (!get_and_check_value(input, "sub_name", output.sub_name, std::string {})) {
        LogError << "failed to get_and_check_value sub_name" << VAR(input);
        return false;
    }
    if (output.sub_name.empty()) {
        output.sub_name = Recognition::kTypeNameMap.at(output.type);
    }

    return true;
}

MAA_RES_NS_END
