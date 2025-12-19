#include "PipelineDumper.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "PipelineParser.h"

MAA_RES_NS_BEGIN

namespace
{

PipelineV2::JRect dump_rect(const cv::Rect& rect)
{
    return { rect.x, rect.y, rect.width, rect.height };
}

template <typename T>
PipelineV2::JTarget dump_target(const T& target)
{
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
}

std::vector<PipelineV2::JTarget> dump_target_obj_array(const std::vector<Action::TargetObj>& vec)
{
    std::vector<PipelineV2::JTarget> result;
    for (const auto& target : vec) {
        result.emplace_back(dump_target(target));
    }
    return result;
}

std::vector<PipelineV2::JRect> dump_rect_array(const std::vector<cv::Rect>& vec)
{
    std::vector<PipelineV2::JRect> result;
    for (const auto& rect : vec) {
        result.emplace_back(dump_rect(rect));
    }
    return result;
}

std::string dump_order_by(MAA_VISION_NS::ResultOrderBy order_by)
{
    static const std::unordered_map<MAA_VISION_NS::ResultOrderBy, std::string> order_by_map = {
        { MAA_VISION_NS::ResultOrderBy::Horizontal, "Horizontal" }, { MAA_VISION_NS::ResultOrderBy::Vertical, "Vertical" },
        { MAA_VISION_NS::ResultOrderBy::Score, "Score" },           { MAA_VISION_NS::ResultOrderBy::Area, "Area" },
        { MAA_VISION_NS::ResultOrderBy::Length, "Length" },         { MAA_VISION_NS::ResultOrderBy::Random, "Random" },
        { MAA_VISION_NS::ResultOrderBy::Expected, "Expected" },
    };
    return order_by_map.at(order_by);
}

PipelineV2::JWaitFreezes dump_wait_freezes(const WaitFreezesParam& param)
{
    return PipelineV2::JWaitFreezes {
        .time = param.time.count(),
        .target = dump_target(param.target),
        .target_offset = dump_rect(param.target.offset),
        .threshold = param.threshold,
        .method = param.method,
        .rate_limit = param.rate_limit.count(),
        .timeout = param.timeout.count(),
    };
}

} // namespace

PipelineV2::JRecognition PipelineDumper::dump_reco(Recognition::Type type, const Recognition::Param& param)
{
    PipelineV2::JRecognition reco;
    reco.type = Recognition::kTypeNameMap.at(type);

    switch (type) {
    case Recognition::Type::DirectHit:
        reco.param = PipelineV2::JDirectHit {};
        break;

    case Recognition::Type::TemplateMatch: {
        const auto& p = std::get<MAA_VISION_NS::TemplateMatcherParam>(param);
        reco.param = PipelineV2::JTemplateMatch {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .template_ = p.template_,
            .threshold = p.thresholds,
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
            .method = p.method,
            .green_mask = p.green_mask,
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

        const auto& p = std::get<MAA_VISION_NS::FeatureMatcherParam>(param);
        reco.param = PipelineV2::JFeatureMatch {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .template_ = p.template_,
            .count = p.count,
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
            .green_mask = p.green_mask,
            .detector = kDetectorNameMap.at(p.detector),
            .ratio = p.ratio,
        };
    } break;

    case Recognition::Type::ColorMatch: {
        const auto& p = std::get<MAA_VISION_NS::ColorMatcherParam>(param);
        std::vector<std::vector<int>> lower;
        std::vector<std::vector<int>> upper;
        for (const auto& r : p.range) {
            lower.emplace_back(r.first);
            upper.emplace_back(r.second);
        }
        reco.param = PipelineV2::JColorMatch {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .method = p.method,
            .lower = std::move(lower),
            .upper = std::move(upper),
            .count = p.count,
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
            .connected = p.connected,
        };
    } break;

    case Recognition::Type::OCR: {
        const auto& p = std::get<MAA_VISION_NS::OCRerParam>(param);
        std::vector<std::string> expected;
        for (const auto& w : p.expected) {
            expected.emplace_back(from_u16(w));
        }
        std::vector<std::pair<std::string, std::string>> replace;
        for (const auto& [old_str, new_str] : p.replace) {
            replace.emplace_back(std::make_pair(from_u16(old_str), from_u16(new_str)));
        }

        reco.param = PipelineV2::JOCR {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .expected = std::move(expected),
            .threshold = p.threshold,
            .replace = std::move(replace),
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
            .only_rec = p.only_rec,
            .model = p.model,
        };
    } break;

    case Recognition::Type::NeuralNetworkClassify: {
        const auto& p = std::get<MAA_VISION_NS::NeuralNetworkClassifierParam>(param);
        reco.param = PipelineV2::JNeuralNetworkClassify {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .labels = p.labels,
            .model = p.model,
            .expected = p.expected,
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
        };
    } break;

    case Recognition::Type::NeuralNetworkDetect: {
        const auto& p = std::get<MAA_VISION_NS::NeuralNetworkDetectorParam>(param);
        reco.param = PipelineV2::JNeuralNetworkDetect {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .labels = p.labels,
            .model = p.model,
            .expected = p.expected,
            .threshold = p.thresholds,
            .order_by = dump_order_by(p.order_by),
            .index = p.result_index,
        };
    } break;

    case Recognition::Type::And: {
        const auto& p = std::get<std::shared_ptr<Recognition::AndParam>>(param);
        if (!p) {
            LogError << "AndParam is null";
            return {};
        }

        std::vector<json::value> all_list;
        for (const auto& sub : p->all_of) {
            auto sub_reco = dump_reco(sub.type, sub.param);
            json::object sub_json = sub_reco.to_json().as_object();
            sub_json["sub_name"] = sub.sub_name;
            all_list.emplace_back(std::move(sub_json));
        }

        reco.param = PipelineV2::JAnd {
            .all_of = std::move(all_list),
            .box_index = p->box_index,
        };
    } break;

    case Recognition::Type::Or: {
        const auto& p = std::get<std::shared_ptr<Recognition::OrParam>>(param);
        if (!p) {
            LogError << "OrParam is null";
            return {};
        }

        std::vector<json::value> any_list;
        for (const auto& sub : p->any_of) {
            auto sub_reco = dump_reco(sub.type, sub.param);
            json::object sub_json = sub_reco.to_json().as_object();
            sub_json["sub_name"] = sub.sub_name;
            any_list.emplace_back(std::move(sub_json));
        }

        reco.param = PipelineV2::JOr {
            .any_of = std::move(any_list),
        };
    } break;

    case Recognition::Type::Custom: {
        const auto& p = std::get<MAA_VISION_NS::CustomRecognitionParam>(param);
        reco.param = PipelineV2::JCustomRecognition {
            .roi = dump_target(p.roi_target),
            .roi_offset = dump_rect(p.roi_target.offset),
            .custom_recognition = p.name,
            .custom_recognition_param = p.custom_param,
        };
    } break;

    default:
        LogError << "Invalid recognition type" << VAR(type);
        return {};
    }

    return reco;
}

json::object PipelineDumper::dump(const PipelineData& pp)
{
    PipelineV2::JPipelineData data;

    data.next = pp.next;
    data.rate_limit = pp.rate_limit.count();
    data.timeout = pp.reco_timeout.count();
    data.on_error = pp.on_error;
    data.anchor = pp.anchor;
    data.inverse = pp.inverse;
    data.enabled = pp.enabled;
    data.pre_delay = pp.pre_delay.count();
    data.post_delay = pp.post_delay.count();
    data.focus = pp.focus;

    data.recognition = dump_reco(pp.reco_type, pp.reco_param);

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
            .contact = param.contact,
        };
    } break;

    case Action::Type::LongPress: {
        const auto& param = std::get<Action::LongPressParam>(pp.action_param);
        data.action.param = PipelineV2::JLongPress {
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
            .duration = param.duration,
            .contact = param.contact,
        };
    } break;

    case Action::Type::Swipe: {
        const auto& param = std::get<Action::SwipeParam>(pp.action_param);
        data.action.param = PipelineV2::JSwipe {
            .starting = 0,
            .begin = dump_target(param.begin),
            .begin_offset = dump_rect(param.begin.offset),
            .end = dump_target_obj_array(param.end),
            .end_offset = dump_rect_array(param.end_offset),
            .end_hold = param.end_hold,
            .duration = param.duration,
            .only_hover = param.only_hover,
            .contact = param.contact,
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
                .end = dump_target_obj_array(s.end),
                .end_offset = dump_rect_array(s.end_offset),
                .end_hold = s.end_hold,
                .duration = s.duration,
                .only_hover = s.only_hover,
                .contact = s.contact,
            });
        }
        data.action.param = std::move(jswipes);
    } break;

    case Action::Type::TouchDown:
    case Action::Type::TouchMove: {
        const auto& param = std::get<Action::TouchParam>(pp.action_param);
        data.action.param = PipelineV2::JTouch {
            .contact = param.contact,
            .target = dump_target(param.target),
            .target_offset = dump_rect(param.target.offset),
            .pressure = param.pressure,
        };
    } break;

    case Action::Type::TouchUp: {
        const auto& param = std::get<Action::TouchUpParam>(pp.action_param);
        data.action.param = PipelineV2::JTouchUp {
            .contact = param.contact,
        };
    } break;

    case Action::Type::ClickKey: {
        const auto& param = std::get<Action::ClickKeyParam>(pp.action_param);
        data.action.param = PipelineV2::JClickKey {
            .key = param.keys,
        };
    } break;

    case Action::Type::LongPressKey: {
        const auto& param = std::get<Action::LongPressKeyParam>(pp.action_param);
        data.action.param = PipelineV2::JLongPressKey {
            .key = param.keys,
            .duration = param.duration,
        };
    } break;

    case Action::Type::KeyDown:
    case Action::Type::KeyUp: {
        const auto& param = std::get<Action::KeyParam>(pp.action_param);
        data.action.param = PipelineV2::JKey {
            .key = param.key,
        };
    } break;

    case Action::Type::InputText: {
        const auto& param = std::get<Action::InputTextParam>(pp.action_param);
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

    case Action::Type::Scroll: {
        const auto& param = std::get<Action::ScrollParam>(pp.action_param);
        data.action.param = PipelineV2::JScroll {
            .dx = param.dx,
            .dy = param.dy,
        };
    } break;

    case Action::Type::StopTask: {
        data.action.param = PipelineV2::JStopTask {};
        break;
    }

    case Action::Type::Command: {
        const auto& param = std::get<Action::CommandParam>(pp.action_param);
        data.action.param = PipelineV2::JCommand {
            .exec = param.exec,
            .args = param.args,
            .detach = param.detach,
        };
    } break;

    case Action::Type::Shell: {
        const auto& param = std::get<Action::ShellParam>(pp.action_param);
        data.action.param = PipelineV2::JShell {
            .cmd = param.cmd,
            .timeout = param.timeout,
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

    data.pre_wait_freezes = dump_wait_freezes(pp.pre_wait_freezes);
    data.post_wait_freezes = dump_wait_freezes(pp.post_wait_freezes);

    data.repeat = pp.repeat;
    data.repeat_delay = pp.repeat_delay.count();
    data.repeat_wait_freezes = dump_wait_freezes(pp.repeat_wait_freezes);

    data.max_hit = pp.max_hit;
    data.attach = pp.attach;

    return data.to_json().as_object();
}

MAA_RES_NS_END
