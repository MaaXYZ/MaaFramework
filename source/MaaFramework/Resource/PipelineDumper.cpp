#include "PipelineDumper.h"

#include "PipelineTypesV2.h"
#include "Utils/Logger.h"
#include "Utils/Codec.h"

MAA_RES_NS_BEGIN

json::object PipelineDumper::dump(const PipelineData& pp)
{
    PipelineV2::JPipelineData data;

    data.next = pp.next;
    data.interrupt = pp.interrupt;
    data.is_sub = pp.is_sub;
    data.rate_limit = pp.rate_limit.count();
    data.timeout = pp.reco_timeout.count();
    data.on_error = pp.on_error;
    data.inverse = pp.inverse;
    data.enabled = pp.enabled;
    data.pre_delay = pp.pre_delay.count();
    data.post_delay = pp.post_delay.count();
    data.focus = pp.focus;

    auto dump_rect = [](const cv::Rect& rect) -> PipelineV2::JRect {
        return { rect.x, rect.y, rect.width, rect.height };
    };

    auto dump_target = [&](const Action::Target& target) -> PipelineV2::JTarget {
        switch (target.type) {
        case Action::Target::Type::Self:
            return true;

        case Action::Target::Type::PreTask:
            return std::get<std::string>(target.param);

        case Action::Target::Type::Region:
            return dump_rect(std::get<cv::Rect>(target.param));

        case Action::Target::Type::Invalid:
        default:
            LogError << "Invalid target type" << VAR(target.type);
            return {};
        }
    };

    auto dump_order_by = [](MAA_VISION_NS::ResultOrderBy order_by) -> std::string {
        static const std::unordered_map<MAA_VISION_NS::ResultOrderBy, std::string> order_by_map = {
            { MAA_VISION_NS::ResultOrderBy::Horizontal, "Horizontal" }, { MAA_VISION_NS::ResultOrderBy::Vertical, "Vertical" },
            { MAA_VISION_NS::ResultOrderBy::Score, "Score" },           { MAA_VISION_NS::ResultOrderBy::Area, "Area" },
            { MAA_VISION_NS::ResultOrderBy::Length, "Length" },         { MAA_VISION_NS::ResultOrderBy::Random, "Random" },
            { MAA_VISION_NS::ResultOrderBy::Expected, "Expected" },
        };
        return order_by_map.at(order_by);
    };

    data.recognition.type = Recognition::kTypeNameMap.at(pp.reco_type);

    switch (pp.reco_type) {
    case Recognition::Type::DirectHit:
        data.recognition.param = PipelineV2::JDirectHit {};
        break;

    case Recognition::Type::TemplateMatch: {
        const auto& param = std::get<MAA_VISION_NS::TemplateMatcherParam>(pp.reco_param);
        data.recognition.param = PipelineV2::JTemplateMatch {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .template_ = param.template_,
            .threshold = param.thresholds,
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
            .method = param.method,
            .green_mask = param.green_mask,
        };
    } break;

    case Recognition::Type::FeatureMatch: {
        static const std::unordered_map<MAA_VISION_NS::FeatureMatcherParam::Detector, std::string> kDetectorNameMap = {
            { MAA_VISION_NS::FeatureMatcherParam::Detector::SIFT, "SIFT" },
            { MAA_VISION_NS::FeatureMatcherParam::Detector::SURF, "SURF" },
            { MAA_VISION_NS::FeatureMatcherParam::Detector::ORB, "ORB" },
            { MAA_VISION_NS::FeatureMatcherParam::Detector::BRISK, "BRISK" },
            { MAA_VISION_NS::FeatureMatcherParam::Detector::KAZE, "KAZE" },
            { MAA_VISION_NS::FeatureMatcherParam::Detector::AKAZE, "AKAZE" },
        };

        const auto& param = std::get<MAA_VISION_NS::FeatureMatcherParam>(pp.reco_param);
        data.recognition.param = PipelineV2::JFeatureMatch {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .template_ = param.template_,
            .count = param.count,
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
            .green_mask = param.green_mask,
            .detector = kDetectorNameMap.at(param.detector),
            .ratio = param.distance_ratio,
        };
    } break;

    case Recognition::Type::ColorMatch: {
        const auto& param = std::get<MAA_VISION_NS::ColorMatcherParam>(pp.reco_param);
        std::vector<std::vector<int>> lower;
        std::vector<std::vector<int>> upper;
        for (const auto& r : param.range) {
            lower.emplace_back(r.first);
            upper.emplace_back(r.second);
        }
        data.recognition.param = PipelineV2::JColorMatch {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .method = param.method,
            .lower = std::move(lower),
            .upper = std::move(upper),
            .count = param.count,
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
            .connected = param.connected,
        };
    } break;

    case Recognition::Type::OCR: {
        const auto& param = std::get<MAA_VISION_NS::OCRerParam>(pp.reco_param);
        std::vector<std::string> expected;
        for (const auto& w : param.expected) {
            expected.emplace_back(from_u16(w));
        }
        std::vector<std::pair<std::string, std::string>> replace;
        for (const auto& [old_str, new_str] : param.replace) {
            replace.emplace_back(std::make_pair(from_u16(old_str), from_u16(new_str)));
        }

        data.recognition.param = PipelineV2::JOCR {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .expected = std::move(expected),
            .threshold = param.threshold,
            .replace = std::move(replace),
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
            .only_rec = param.only_rec,
            .model = param.model,
        };
    } break;

    case Recognition::Type::NeuralNetworkClassify: {
        const auto& param = std::get<MAA_VISION_NS::NeuralNetworkClassifierParam>(pp.reco_param);
        data.recognition.param = PipelineV2::JNeuralNetworkClassify {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .labels = param.labels,
            .model = param.model,
            .expected = param.expected,
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
        };
    } break;

    case Recognition::Type::NeuralNetworkDetect: {
        const auto& param = std::get<MAA_VISION_NS::NeuralNetworkDetectorParam>(pp.reco_param);
        data.recognition.param = PipelineV2::JNeuralNetworkDetect {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .labels = param.labels,
            .model = param.model,
            .expected = param.expected,
            .threshold = param.thresholds,
            .order_by = dump_order_by(param.order_by),
            .index = param.result_index,
        };
    } break;
    case Recognition::Type::Custom: {
        const auto& param = std::get<MAA_VISION_NS::CustomRecognitionParam>(pp.reco_param);
        data.recognition.param = PipelineV2::JCustomRecognition {
            .roi = dump_target(param.roi_target),
            .roi_offset = dump_rect(param.roi_target.offset),
            .custom_recognition = param.name,
            .custom_recognition_param = param.custom_param,
        };
    } break;

    default:
        LogError << "Invalid recognition type" << VAR(pp.reco_type);
        return {};
    }

    data.action.type = Action::kTypeNameMap.at(pp.action_type);

    switch (pp.action_type) {
    case Action::Type::DoNothing:
        data.action.param = PipelineV2::JDoNothing {};
        break;

    case Action::Type::Click: {
        const auto& param = std::get<Action::ClickParam>(pp.action_param);
        data.action.param = PipelineV2::JClick {
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
        };
    } break;

    case Action::Type::LongPress: {
        const auto& param = std::get<Action::LongPressParam>(pp.action_param);
        data.action.param = PipelineV2::JLongPress {
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
            .duration = param.duration,
        };
    } break;

    case Action::Type::Swipe: {
        const auto& param = std::get<Action::SwipeParam>(pp.action_param);
        data.action.param = PipelineV2::JSwipe {
            .starting = 0,
            .begin = dump_target(param.begin),
            .begin_offset = dump_rect(param.begin.offset),
            .end = dump_target(param.end),
            .end_offset = dump_rect(param.end.offset),
            .duration = param.duration,
        };
    } break;

    case Action::Type::MultiSwipe: {
        const auto& param = std::get<Action::MultiSwipeParam>(pp.action_param);
        PipelineV2::JMultiSwipe jswipes;
        for (const auto& s : param.swipes) {
            jswipes.swipes.emplace_back(PipelineV2::JSwipe {
                .starting = s.starting,
                .begin = dump_target(s.begin),
                .begin_offset = dump_rect(s.begin.offset),
                .end = dump_target(s.end),
                .end_offset = dump_rect(s.end.offset),
                .duration = s.duration,
            });
        }
        data.action.param = std::move(jswipes);
    } break;

    case Action::Type::Key: {
        const auto& param = std::get<Action::KeyParam>(pp.action_param);
        data.action.param = PipelineV2::JKey {
            .key = param.keys,
        };
    } break;

    case Action::Type::Text: {
        const auto& param = std::get<Action::TextParam>(pp.action_param);
        data.action.param = PipelineV2::JInputText {
            .input_text = param.text,
        };
    } break;

    case Action::Type::StartApp: {
        const auto& param = std::get<Action::AppParam>(pp.action_param);
        data.action.param = PipelineV2::JStartApp {
            .package = param.package,
        };
    } break;

    case Action::Type::StopApp: {
        const auto& param = std::get<Action::AppParam>(pp.action_param);
        data.action.param = PipelineV2::JStopApp {
            .package = param.package,
        };
    } break;

    case Action::Type::Command: {
        const auto& param = std::get<Action::CommandParam>(pp.action_param);
        data.action.param = PipelineV2::JCommand {
            .exec = param.exec,
            .args = param.args,
            .detach = param.detach,
        };
    } break;

    case Action::Type::Custom: {
        const auto& param = std::get<Action::CustomParam>(pp.action_param);
        data.action.param = PipelineV2::JCustomAction {
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
            .custom_action = param.name,
            .custom_action_param = param.custom_param,
        };
    } break;

    default:
        LogError << "Invalid action type" << VAR(pp.action_type);
        return {};
    }

    auto dump_wait_freezes = [&](const WaitFreezesParam& param) -> PipelineV2::JWaitFreezes {
        return PipelineV2::JWaitFreezes {
            .time = param.time.count(),
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
            .threshold = param.threshold,
            .method = param.method,
            .rate_limit = param.rate_limit.count(),
            .timeout = param.timeout.count(),
        };
    };

    data.pre_wait_freezes = dump_wait_freezes(pp.pre_wait_freezes);
    data.post_wait_freezes = dump_wait_freezes(pp.post_wait_freezes);

    return data.to_json().as_object();
}

MAA_RES_NS_END