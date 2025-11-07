#include "Actuator.h"

#include "CommandAction.h"
#include "Controller/ControllerAgent.h"
#include "CustomAction.h"
#include "MaaUtils/Logger.h"
#include "Vision/TemplateComparator.h"

MAA_TASK_NS_BEGIN

std::mt19937 Actuator::rand_engine_(std::random_device {}());

Actuator::Actuator(Tasker* tasker, Context& context)
    : tasker_(tasker)
    , context_(context)
{
}

bool Actuator::run(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry)
{
    using namespace MAA_RES_NS::Action;
    LogFunc << VAR(pipeline_data.name);

    if (pipeline_data.action_type == Type::Invalid) {
        LogDebug << "invalid action";
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
    case Type::LongPress:
        ret = long_press(std::get<LongPressParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::Swipe:
        ret = swipe(std::get<SwipeParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::MultiSwipe:
        ret = multi_swipe(std::get<MultiSwipeParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::TouchDown:
        ret = touch_down(std::get<TouchParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::TouchMove:
        ret = touch_move(std::get<TouchParam>(pipeline_data.action_param), reco_hit);
        break;
    case Type::TouchUp:
        ret = touch_up(std::get<TouchUpParam>(pipeline_data.action_param));
        break;
    case Type::ClickKey:
        ret = click_key(std::get<ClickKeyParam>(pipeline_data.action_param));
        break;
    case Type::LongPressKey:
        ret = long_press_key(std::get<LongPressKeyParam>(pipeline_data.action_param));
        break;
    case Type::KeyDown:
        ret = key_down(std::get<KeyParam>(pipeline_data.action_param));
        break;
    case Type::KeyUp:
        ret = key_up(std::get<KeyParam>(pipeline_data.action_param));
        break;
    case Type::InputText:
        ret = input_text(std::get<InputTextParam>(pipeline_data.action_param));
        break;
    case Type::StartApp:
        ret = start_app(std::get<AppParam>(pipeline_data.action_param));
        break;
    case Type::StopApp:
        ret = stop_app(std::get<AppParam>(pipeline_data.action_param));
        break;
    case Type::Command:
        ret = command(std::get<CommandParam>(pipeline_data.action_param), reco_hit, pipeline_data.name, entry);
        break;
    case Type::Custom:
        ret = custom_action(std::get<CustomParam>(pipeline_data.action_param), reco_hit, reco_id, pipeline_data.name);
        break;
    case Type::StopTask:
        LogInfo << "Action: StopTask";
        context_.need_to_stop() = true;
        ret = true;
        break;
    default:
        ret = false;
        LogError << "Unknown action" << VAR(static_cast<int>(pipeline_data.action_type));
        break;
    }

    wait_freezes(pipeline_data.post_wait_freezes, reco_hit);
    sleep(pipeline_data.post_delay);

    return ret;
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

bool Actuator::click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    return controller()->click({ .point = point });
}

bool Actuator::long_press(const MAA_RES_NS::Action::LongPressParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));

    return controller()->long_press({ .point = point, .duration = param.duration });
}

bool Actuator::swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
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

    return controller()->swipe(
        { .begin = begin,
          .end = std::move(end),
          .end_hold = param.end_hold,
          .duration = param.duration,
          .only_hover = param.only_hover,
          .starting = param.starting });
}

bool Actuator::multi_swipe(const MAA_RES_NS::Action::MultiSwipeParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
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
              .starting = swipe.starting });
    }

    return controller()->multi_swipe({ .swipes = std::move(swipes) });
}

bool Actuator::touch_down(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    return controller()->touch_down({ .contact = static_cast<int>(param.contact), .point = point, .pressure = param.pressure });
}

bool Actuator::touch_move(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    cv::Point point = rand_point(get_target_rect(param.target, box));
    return controller()->touch_move({ .contact = static_cast<int>(param.contact), .point = point, .pressure = param.pressure });
}

bool Actuator::touch_up(const MAA_RES_NS::Action::TouchUpParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->touch_up({ .contact = static_cast<int>(param.contact) });
}

bool Actuator::click_key(const MAA_RES_NS::Action::ClickKeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->click_key({ .keycode = param.keys });
}

bool Actuator::long_press_key(const MAA_RES_NS::Action::LongPressKeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->long_press_key({ .keycode = param.keys, .duration = param.duration });
}

bool Actuator::key_down(const MAA_RES_NS::Action::KeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->key_down({ .keycode = { param.key } });
}

bool Actuator::key_up(const MAA_RES_NS::Action::KeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->key_up({ .keycode = { param.key } });
}

bool Actuator::input_text(const MAA_RES_NS::Action::InputTextParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->input_text({ .text = param.text });
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

    LogFunc << "Wait freezes:" << VAR(param.time) << VAR(param.rate_limit) << VAR(param.timeout) << VAR(param.threshold)
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
            LogError << "Wait freezes timeout" << VAR(duration_since(start_clock)) << VAR(param.timeout);
            break;
        }

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            break;
        }

        TemplateComparator comparator(pre_image, cur_image, roi, comp_param);

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

    return controller()->start_app({ .package = param.package });
}

bool Actuator::stop_app(const MAA_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    return controller()->stop_app({ .package = param.package });
}

bool Actuator::command(
    const MAA_RES_NS::Action::CommandParam& param,
    const cv::Rect& box,
    const std::string& name,
    const std::string& entry)
{
    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }
    auto* resource = tasker_ ? tasker_->resource() : nullptr;
    if (!resource) {
        LogError << "Resource is null";
        return false;
    }

    CommandAction::Runtime rt {
        .resource_paths = resource->paths(),
        .entry = entry,
        .node = name,
        .image = controller()->cached_image(),
        .box = box,
    };
    return CommandAction().run(param, rt);
}

bool Actuator::custom_action(const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, MaaRecoId reco_id, const std::string& name)
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
    cv::Rect rect = get_target_rect(param.target, box);
    return CustomAction::run(context_, name, session, param, reco_id, rect);
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
    LogFunc << ms;

    std::this_thread::sleep_for(ms);
}

MAA_TASK_NS_END
