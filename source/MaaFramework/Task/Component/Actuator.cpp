#include "Actuator.h"

#include "Controller/ControllerAgent.h"
#include "CustomAction.h"
#include "Utils/Logger.h"
#include "Vision/TemplateComparator.h"

MAA_TASK_NS_BEGIN

Actuator::Actuator(Tasker* tasker, Context& context)
    : tasker_(tasker)
    , context_(context)
{
}

bool Actuator::run(const cv::Rect& reco_hit, const json::value& reco_detail, const PipelineData& pipeline_data)
{
    using namespace MAA_RES_NS::Action;
    LogFunc << VAR(pipeline_data.name);

    if (pipeline_data.action_type == Type::Invalid) {
        LogTrace << "invalid action";
        return false;
    }

    wait_freezes(pipeline_data.pre_wait_freezes, reco_hit);
    sleep(pipeline_data.pre_delay);

    bool ret = false;
    switch (pipeline_data.action_type) {
    case Type::DoNothing:
        ret = true;
        break;
    case Type::Click:
        ret = click(std::get<ClickParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::Swipe:
        ret = swipe(std::get<SwipeParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::Key:
        ret = press_key(std::get<KeyParam>(pipeline_data.action_param));
        break;
    case Type::Text:
        ret = input_text(std::get<TextParam>(pipeline_data.action_param));
        break;
    case Type::StartApp:
        ret = start_app(std::get<AppParam>(pipeline_data.action_param));
        break;
    case Type::StopApp:
        ret = stop_app(std::get<AppParam>(pipeline_data.action_param));
        break;
    case Type::Custom:
        ret = custom_action(std::get<CustomParam>(pipeline_data.action_param), reco_hit, reco_detail);
        break;
    case Type::StopTask:
        LogInfo << "Action: StopTask";
        return false;
    default:
        ret = false;
        LogError << "Unknown action" << VAR(static_cast<int>(pipeline_data.action_type));
        break;
    }

    wait_freezes(pipeline_data.post_wait_freezes, reco_hit);
    sleep(pipeline_data.post_delay);

    return ret;
}

bool Actuator::click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Rect rect = get_target_rect(param.target, box);

    return controller()->click(rect);
}

bool Actuator::swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Rect begin = get_target_rect(param.begin, box);
    cv::Rect end = get_target_rect(param.end, box);

    return controller()->swipe(begin, end, param.duration);
}

bool Actuator::press_key(const MAA_RES_NS::Action::KeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }
    bool ret = true;
    for (const auto& key : param.keys) {
        ret &= controller()->press_key(key);
    }
    return ret;
}

bool Actuator::input_text(const MAA_RES_NS::Action::TextParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }
    return controller()->input_text(param.text);
}

void Actuator::wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return;
    }

    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    LogFunc << "Wait freezes:" << VAR(param.time) << VAR(param.threshold) << VAR(param.method);

    cv::Rect target = get_target_rect(param.target, box);

    auto screencap_clock = std::chrono::steady_clock::now();
    cv::Mat pre_image = controller()->screencap();

    TemplateComparatorParam comp_param {
        .roi = { target },
        .threshold = param.threshold,
        .method = param.method,
    };

    auto pre_image_clock = std::chrono::steady_clock::now();

    while (true) {
        std::this_thread::sleep_until(screencap_clock + param.rate_limit);

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            break;
        }

        TemplateComparator comparator(pre_image, cur_image, comp_param);

        if (!comparator.best_result()) {
            pre_image = cur_image;
            pre_image_clock = std::chrono::steady_clock::now();
            continue;
        }

        if (duration_since(pre_image_clock) > param.time) {
            break;
        }
    }
}

bool Actuator::start_app(const MAA_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }
    using namespace MAA_VISION_NS;

    if (param.package.empty()) {
        return controller()->start_app();
    }
    return controller()->start_app(param.package);
}

bool Actuator::stop_app(const MAA_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }
    using namespace MAA_VISION_NS;

    if (param.package.empty()) {
        return controller()->stop_app();
    }
    return controller()->stop_app(param.package);
}

bool Actuator::custom_action(const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, const json::value& reco_detail)
{
    if (!tasker_) {
        LogError << "tasker_ is null";
        return false;
    }
    if (!tasker_->resource()) {
        LogError << "resource is null";
        return false;
    }
    auto session = tasker_->resource()->custom_action(param.name);
    return CustomAction(param.name, session).run(context_, param, box, reco_detail);
}

cv::Rect Actuator::get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& box)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    using namespace MAA_RES_NS::Action;

    cv::Rect raw {};
    switch (target.type) {
    case Target::Type::Self:
        raw = box;
        break;
    case Target::Type::PreTask: {
        auto& cache = tasker_->runtime_cache();
        std::string name = std::get<std::string>(target.param);
        MaaNodeId node_id = cache.get_latest_node(name).value_or(MaaInvalidId);
        NodeDetail node_detail = cache.get_node_detail(node_id).value_or(NodeDetail {});
        RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult {});
        raw = reco_result.box.value_or(cv::Rect {});
        LogTrace << "pre task" << VAR(name) << VAR(raw);
    } break;
    case Target::Type::Region:
        raw = std::get<cv::Rect>(target.param);
        break;
    default:
        LogError << "Unknown target" << VAR(static_cast<int>(target.type));
        return {};
    }

    return cv::Rect { raw.x + target.offset.x,
                      raw.y + target.offset.y,
                      raw.width + target.offset.width,
                      raw.height + target.offset.height };
}

MAA_CTRL_NS::ControllerAgent* Actuator::controller()
{
    return tasker_ ? tasker_->controller() : nullptr;
}

void Actuator::sleep(unsigned ms) const
{
    sleep(std::chrono::milliseconds(ms));
}

void Actuator::sleep(std::chrono::milliseconds ms) const
{
    LogFunc << ms;

    std::this_thread::sleep_for(ms);
}

MAA_TASK_NS_END
