#include "Actuator.h"

#include "Controller/ControllerAgent.h"
#include "Global/UniqueResultBank.h"
#include "Task/CustomAction.h"
#include "Utils/Logger.h"
#include "Vision/TemplateComparator.h"

MAA_TASK_NS_BEGIN

Actuator::Actuator(InstanceInternalAPI* inst, const PreTaskBoxes& boxes)
    : inst_(inst)
    , pre_task_boxes_(boxes)
{
}

bool Actuator::run(
    const Recognizer::Hit& reco_hit,
    const json::value& reco_detail,
    const TaskData& task_data)
{
    using namespace MAA_RES_NS::Action;
    LogFunc << VAR(task_data.name);

    wait_freezes(task_data.pre_wait_freezes, reco_hit);
    sleep(task_data.pre_delay);

    bool ret = false;
    switch (task_data.action_type) {
    case Type::DoNothing:
        ret = true;
        break;
    case Type::Click:
        ret = click(std::get<ClickParam>(task_data.action_param), reco_hit);
        break;
    case Type::Swipe:
        ret = swipe(std::get<SwipeParam>(task_data.action_param), reco_hit);
        break;
    case Type::Key:
        ret = press_key(std::get<KeyParam>(task_data.action_param));
        break;
    case Type::Text:
        ret = input_text(std::get<TextParam>(task_data.action_param));
        break;
    case Type::StartApp:
        ret = start_app(std::get<AppParam>(task_data.action_param));
        break;
    case Type::StopApp:
        ret = stop_app(std::get<AppParam>(task_data.action_param));
        break;
    case Type::Custom:
        ret = custom_action(
            task_data.name,
            std::get<CustomParam>(task_data.action_param),
            reco_hit,
            reco_detail);
        break;
    case Type::StopTask:
        LogInfo << "Action: StopTask";
        return false;
    default:
        ret = false;
        LogError << "Unknown action" << VAR(static_cast<int>(task_data.action_type));
        break;
    }

    wait_freezes(task_data.post_wait_freezes, reco_hit);
    sleep(task_data.post_delay);

    return ret;
}

bool Actuator::click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Rect rect = get_target_rect(param.target, cur_box);

    return controller()->click(rect);
}

bool Actuator::swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Rect begin = get_target_rect(param.begin, cur_box);
    cv::Rect end = get_target_rect(param.end, cur_box);

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

void Actuator::wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& cur_box)
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

    cv::Rect target = get_target_rect(param.target, cur_box);
    cv::Mat pre_image = controller()->screencap();

    TemplateComparatorParam comp_param {
        .roi = { target },
        .threshold = param.threshold,
        .method = param.method,
    };

    auto pre_time = std::chrono::steady_clock::now();

    while (true) {
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            break;
        }

        TemplateComparator comparator(pre_image, cur_image, comp_param);

        if (!comparator.best_result()) {
            pre_image = cur_image;
            pre_time = std::chrono::steady_clock::now();
            continue;
        }

        if (duration_since(pre_time) > param.time) {
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

bool Actuator::custom_action(
    const std::string& task_name,
    const MAA_RES_NS::Action::CustomParam& param,
    const cv::Rect& cur_box,
    const json::value& cur_rec_detail)
{
    if (!inst_) {
        LogError << "Inst is null";
        return false;
    }
    auto* session = inst_->custom_action_session(param.name);
    if (!session) {
        LogError << "Custom task not found" << VAR(param.name);
        return false;
    }

    return CustomAction(*session, inst_).run(task_name, param, cur_box, cur_rec_detail);
}

cv::Rect Actuator::get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& cur_box)
{
    using namespace MAA_RES_NS::Action;

    cv::Rect raw {};
    switch (target.type) {
    case Target::Type::Self:
        raw = cur_box;
        break;
    case Target::Type::PreTask: {
        const std::string& pre_task_name = std::get<std::string>(target.param);
        if (auto it = pre_task_boxes_.find(pre_task_name); it == pre_task_boxes_.end()) {
            LogError << "Pre task not found" << VAR(pre_task_name);
        }
        else {
            raw = it->second;
        }
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