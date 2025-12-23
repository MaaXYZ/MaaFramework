#include "Actuator.h"

#include "CommandAction.h"
#include "Controller/ControllerAgent.h"
#include "CustomAction.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Vision/TemplateComparator.h"

MAA_TASK_NS_BEGIN

std::mt19937 Actuator::rand_engine_(std::random_device {}());

Actuator::Actuator(Tasker* tasker, Context& context)
    : tasker_(tasker)
    , context_(context)
{
}

ActionResult Actuator::run(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry)
{
    using namespace MAA_RES_NS::Action;
    LogTrace << VAR(pipeline_data.name);

    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    if (pipeline_data.action_type == Type::Invalid) {
        LogDebug << "invalid action";
        return {};
    }

    wait_freezes(pipeline_data.pre_wait_freezes, reco_hit);
    sleep(pipeline_data.pre_delay);

    auto& rt_cache = tasker_->runtime_cache();
    ActionResult result;

    for (uint i = 0; i < pipeline_data.repeat; ++i) {
        if (i > 0) {
            wait_freezes(pipeline_data.repeat_wait_freezes, reco_hit);
            sleep(pipeline_data.repeat_delay);
        }

        if (context_.need_to_stop()) {
            return {};
        }

        result = execute_action(reco_hit, reco_id, pipeline_data, entry);
        LogInfo << "action" << VAR(i) << VAR(pipeline_data.repeat) << VAR(result);
        rt_cache.set_action_detail(result.action_id, result);

        if (context_.need_to_stop()) {
            return {};
        }
    }

    wait_freezes(pipeline_data.post_wait_freezes, reco_hit);
    sleep(pipeline_data.post_delay);

    return result;
}

ActionResult
    Actuator::execute_action(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry)
{
    using namespace MAA_RES_NS::Action;

    switch (pipeline_data.action_type) {
    case Type::DoNothing:
        return do_nothing(pipeline_data.name);

    case Type::Click:
        return click(std::get<ClickParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::LongPress:
        return long_press(std::get<LongPressParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::Swipe:
        return swipe(std::get<SwipeParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::MultiSwipe:
        return multi_swipe(std::get<MultiSwipeParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::TouchDown:
        return touch_down(std::get<TouchParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::TouchMove:
        return touch_move(std::get<TouchParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);

    case Type::TouchUp:
        return touch_up(std::get<TouchUpParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::ClickKey:
        return click_key(std::get<ClickKeyParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::LongPressKey:
        return long_press_key(std::get<LongPressKeyParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::KeyDown:
        return key_down(std::get<KeyParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::KeyUp:
        return key_up(std::get<KeyParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::InputText:
        return input_text(std::get<InputTextParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::StartApp:
        return start_app(std::get<AppParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::StopApp:
        return stop_app(std::get<AppParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::Scroll:
        return scroll(std::get<ScrollParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::StopTask:
        return stop_task(pipeline_data.name);

    case Type::Command:
        return command(std::get<CommandParam>(pipeline_data.action_param), reco_hit, pipeline_data.name, entry);

    case Type::Shell:
        return shell(std::get<ShellParam>(pipeline_data.action_param), pipeline_data.name);

    case Type::Custom:
        return custom_action(std::get<CustomParam>(pipeline_data.action_param), reco_hit, reco_id, pipeline_data.name);

    default:
        LogError << "Unknown action" << VAR(static_cast<int>(pipeline_data.action_type));
        return {};
    }
}

cv::Point Actuator::rand_point(const cv::Rect& r)
{
    // 过小矩形直接返回中心点，避免死循环
    if (r.width <= 2 || r.height <= 2) {
        return { r.x + r.width / 2, r.y + r.height / 2 };
    }

    constexpr double kStdDevFactor = 3.0;

    const double std_dev_x = r.width / kStdDevFactor;
    const double std_dev_y = r.height / kStdDevFactor;

    std::normal_distribution<double> dist_x(r.x + r.width / 2.0, std_dev_x);
    std::normal_distribution<double> dist_y(r.y + r.height / 2.0, std_dev_y);

    // 优先进行有限次拒绝采样
    constexpr int kMaxAttempts = 8;
    for (int i = 0; i < kMaxAttempts; ++i) {
        const int x = static_cast<int>(std::round(dist_x(rand_engine_)));
        const int y = static_cast<int>(std::round(dist_y(rand_engine_)));
        const cv::Point sampled { x, y };
        if (!r.contains(sampled)) {
            continue;
        }

        return sampled;
    }

    LogWarn << "Too many sampling attempts";
    // 返回中心点
    return { r.x + r.width / 2, r.y + r.height / 2 };
}

ActionResult Actuator::click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    MAA_CTRL_NS::ClickParam ctrl_param { .point = point, .contact = static_cast<int>(param.contact) };
    bool ret = controller()->click(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Click",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::long_press(const MAA_RES_NS::Action::LongPressParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    MAA_CTRL_NS::LongPressParam ctrl_param { .point = point, .duration = param.duration, .contact = static_cast<int>(param.contact) };
    bool ret = controller()->long_press(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "LongPress",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    cv::Point begin = rand_point(get_target_rect(param.begin, box));

    std::vector<cv::Point> end;
    for (size_t i = 0; i < param.end.size(); ++i) {
        const auto& e = param.end.at(i);
        cv::Rect end_offset = param.end_offset.empty()      ? cv::Rect {}
                              : i < param.end_offset.size() ? param.end_offset.at(i)
                                                            : param.end_offset.back();
        MAA_RES_NS::Action::Target end_target { .type = e.type, .param = e.param, .offset = end_offset };
        cv::Point p = rand_point(get_target_rect(end_target, box));
        end.emplace_back(p);
    }

    MAA_CTRL_NS::SwipeParam ctrl_param { .begin = begin,
                                         .end = std::move(end),
                                         .end_hold = param.end_hold,
                                         .duration = param.duration,
                                         .only_hover = param.only_hover,
                                         .starting = param.starting,
                                         .contact = static_cast<int>(param.contact) };
    bool ret = controller()->swipe(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Swipe",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::multi_swipe(const MAA_RES_NS::Action::MultiSwipeParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    std::vector<MAA_CTRL_NS::SwipeParam> swipes;
    for (const auto& swipe : param.swipes) {
        cv::Point begin = rand_point(get_target_rect(swipe.begin, box));

        std::vector<cv::Point> end;
        for (size_t i = 0; i < swipe.end.size(); ++i) {
            const auto& e = swipe.end.at(i);
            cv::Rect end_offset = swipe.end_offset.empty()      ? cv::Rect {}
                                  : i < swipe.end_offset.size() ? swipe.end_offset.at(i)
                                                                : swipe.end_offset.back();
            MAA_RES_NS::Action::Target end_target { .type = e.type, .param = e.param, .offset = end_offset };
            cv::Point p = rand_point(get_target_rect(end_target, box));
            end.emplace_back(p);
        }
        swipes.push_back(
            { .begin = begin,
              .end = std::move(end),
              .end_hold = swipe.end_hold,
              .duration = swipe.duration,
              .only_hover = swipe.only_hover,
              .starting = swipe.starting,
              .contact = static_cast<int>(swipe.contact) });
    }

    MAA_CTRL_NS::MultiSwipeParam ctrl_param { .swipes = std::move(swipes) };
    bool ret = controller()->multi_swipe(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "MultiSwipe",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::touch_down(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    MAA_CTRL_NS::TouchParam ctrl_param { .contact = static_cast<int>(param.contact), .point = point, .pressure = param.pressure };
    bool ret = controller()->touch_down(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "TouchDown",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::touch_move(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    MAA_CTRL_NS::TouchParam ctrl_param { .contact = static_cast<int>(param.contact), .point = point, .pressure = param.pressure };
    bool ret = controller()->touch_move(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "TouchMove",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::touch_up(const MAA_RES_NS::Action::TouchUpParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::TouchParam ctrl_param { .contact = static_cast<int>(param.contact), .point = {}, .pressure = 0 };
    bool ret = controller()->touch_up(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "TouchUp",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::click_key(const MAA_RES_NS::Action::ClickKeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = param.keys };
    bool ret = controller()->click_key(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "ClickKey",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::long_press_key(const MAA_RES_NS::Action::LongPressKeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::LongPressKeyParam ctrl_param { .keycode = param.keys, .duration = param.duration };
    bool ret = controller()->long_press_key(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "LongPressKey",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::key_down(const MAA_RES_NS::Action::KeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = { param.key } };
    bool ret = controller()->key_down(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "KeyDown",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::key_up(const MAA_RES_NS::Action::KeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = { param.key } };
    bool ret = controller()->key_up(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "KeyUp",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::input_text(const MAA_RES_NS::Action::InputTextParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::InputTextParam ctrl_param { .text = param.text };
    bool ret = controller()->input_text(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "InputText",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::scroll(const MAA_RES_NS::Action::ScrollParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::ScrollParam ctrl_param { .dx = param.dx, .dy = param.dy };
    bool ret = controller()->scroll(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Scroll",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::shell(const MAA_RES_NS::Action::ShellParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    std::string output;
    bool ret = controller()->shell(param.cmd, output, param.timeout);

    LogDebug << "Shell command executed" << VAR(param.cmd) << VAR(param.timeout) << VAR(ret);
    if (!output.empty()) {
        LogTrace << "Shell output:" << output;
    }

    json::object detail { { "cmd", param.cmd }, { "timeout", param.timeout }, { "success", ret }, { "output", output } };

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Shell",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(detail),
    };
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

    LogTrace << "Wait freezes:" << VAR(param.time) << VAR(param.rate_limit) << VAR(param.timeout) << VAR(param.threshold)
             << VAR(param.method);

    auto rate_limit = std::min(param.rate_limit, param.time);

    auto screencap_clock = std::chrono::steady_clock::now();
    cv::Mat pre_image = controller()->screencap();

    cv::Rect roi = get_target_rect(param.target, box);
    TemplateComparatorParam comp_param {
        .threshold = param.threshold,
        .method = param.method,
    };

    const auto start_clock = std::chrono::steady_clock::now();
    auto pre_image_clock = start_clock;

    while (true) {
        LogDebug << "sleep_until" << VAR(rate_limit);
        std::this_thread::sleep_until(screencap_clock + rate_limit);

        if (duration_since(start_clock) > param.timeout) {
            LogWarn << "Wait freezes timeout" << VAR(duration_since(start_clock)) << VAR(param.timeout);
            break;
        }

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            break;
        }

        TemplateComparator comparator(pre_image, cur_image, { roi }, comp_param);

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

ActionResult Actuator::start_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::AppParam ctrl_param { .package = param.package };
    bool ret = controller()->start_app(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "StartApp",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::stop_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }

    MAA_CTRL_NS::AppParam ctrl_param { .package = param.package };
    bool ret = controller()->stop_app(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "StopApp",
        .box = cv::Rect {},
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult
    Actuator::command(const MAA_RES_NS::Action::CommandParam& param, const cv::Rect& box, const std::string& name, const std::string& entry)
{
    if (!controller()) {
        LogError << "Controller is null";
        return {};
    }
    auto* resource = tasker_ ? tasker_->resource() : nullptr;
    if (!resource) {
        LogError << "Resource is null";
        return {};
    }

    CommandAction::Runtime rt {
        .resource_paths = resource->paths(),
        .entry = entry,
        .node = name,
        .image = controller()->cached_image(),
        .box = box,
    };
    bool ret = CommandAction().run(param, rt);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Command",
        .box = box,
        .success = ret,
        .detail = json::object(),
    };
}

ActionResult
    Actuator::custom_action(const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, MaaRecoId reco_id, const std::string& name)
{
    if (!tasker_) {
        LogError << "tasker_ is null";
        return {};
    }
    if (!tasker_->resource()) {
        LogError << "resource is null";
        return {};
    }
    auto session = tasker_->resource()->custom_action(param.name);
    cv::Rect rect = get_target_rect(param.target, box);
    bool ret = CustomAction::run(context_, name, session, param, reco_id, rect);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Custom",
        .box = box,
        .success = ret,
        .detail = json::object(),
    };
}

ActionResult Actuator::do_nothing(const std::string& name)
{
    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "DoNothing",
        .box = cv::Rect {},
        .success = true,
        .detail = json::object(),
    };
}

ActionResult Actuator::stop_task(const std::string& name)
{
    LogInfo << "Action: StopTask";
    context_.need_to_stop() = true;

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "StopTask",
        .box = cv::Rect {},
        .success = true,
        .detail = json::object(),
    };
}

cv::Rect Actuator::get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& box)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    if (!tasker_->controller()) {
        LogError << "controller is null";
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
        LogDebug << "pre task" << VAR(name) << VAR(raw);
    } break;
    case Target::Type::Region:
        raw = std::get<cv::Rect>(target.param);
        break;
    default:
        LogError << "Unknown target" << VAR(static_cast<int>(target.type));
        return {};
    }

    auto image = controller()->cached_image();

    int x = std::clamp(raw.x + target.offset.x, 0, image.cols);
    int y = std::clamp(raw.y + target.offset.y, 0, image.rows);
    int width = std::clamp(raw.width + target.offset.width, 0, image.cols - x);
    int height = std::clamp(raw.height + target.offset.height, 0, image.rows - y);

    return cv::Rect(x, y, width, height);
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
    LogDebug << ms;

    std::this_thread::sleep_for(ms);
}

MAA_TASK_NS_END
