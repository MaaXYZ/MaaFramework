#include "Actuator.h"

#include "CommandAction.h"
#include "Controller/ControllerAgent.h"
#include "CustomAction.h"
#include "Global/OptionMgr.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Time.hpp"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

std::mt19937 Actuator::rand_engine_(std::random_device { }());

Actuator::Actuator(Tasker* tasker, Context& context)
    : tasker_(tasker)
    , context_(context)
    , helper_(&context)
{
}

ActionResult Actuator::run(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry)
{
    using namespace MAA_RES_NS::Action;
    LogTrace << VAR(pipeline_data.name);

    if (!tasker_) {
        LogError << "tasker is null";
        return { };
    }

    if (pipeline_data.action_type == Type::Invalid) {
        LogDebug << "invalid action";
        return { };
    }

    ActionResult result;

    switch (pipeline_data.action_type) {
    case Type::DoNothing:
        result = do_nothing(pipeline_data.name);
        break;

    case Type::Click:
        result = click(std::get<ClickParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::LongPress:
        result = long_press(std::get<LongPressParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::Swipe:
        result = swipe(std::get<SwipeParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::MultiSwipe:
        result = multi_swipe(std::get<MultiSwipeParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::TouchDown:
        result = touch_down(std::get<TouchParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::TouchMove:
        result = touch_move(std::get<TouchParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::TouchUp:
        result = touch_up(std::get<TouchUpParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::ClickKey:
        result = click_key(std::get<ClickKeyParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::LongPressKey:
        result = long_press_key(std::get<LongPressKeyParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::KeyDown:
        result = key_down(std::get<KeyParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::KeyUp:
        result = key_up(std::get<KeyParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::InputText:
        result = input_text(std::get<InputTextParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::StartApp:
        result = start_app(std::get<AppParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::StopApp:
        result = stop_app(std::get<AppParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::Scroll:
        result = scroll(std::get<ScrollParam>(pipeline_data.action_param), reco_hit, pipeline_data.name);
        break;

    case Type::StopTask:
        result = stop_task(pipeline_data.name);
        break;

    case Type::Command:
        result = command(std::get<CommandParam>(pipeline_data.action_param), reco_hit, pipeline_data.name, entry);
        break;

    case Type::Shell:
        result = shell(std::get<ShellParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::Screencap:
        result = screencap(std::get<ScreencapParam>(pipeline_data.action_param), pipeline_data.name);
        break;

    case Type::Custom:
        result = custom_action(std::get<CustomParam>(pipeline_data.action_param), reco_hit, reco_id, pipeline_data.name);
        break;

    default:
        LogError << "Unknown action" << VAR(static_cast<int>(pipeline_data.action_type));
        return { };
    }

    tasker_->runtime_cache().set_action_detail(result.action_id, result);

    return result;
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
        return { };
    }

    auto target_rect = helper_.get_target_rect(param.target, box);
    if (target_rect.empty()) {
        LogError << "failed to get target rect" << VAR(name);
        return { };
    }
    cv::Point point = rand_point(target_rect);
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
        return { };
    }

    auto target_rect = helper_.get_target_rect(param.target, box);
    if (target_rect.empty()) {
        LogError << "failed to get target rect" << VAR(name);
        return { };
    }
    cv::Point point = rand_point(target_rect);
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
        return { };
    }

    auto begin_rect = helper_.get_target_rect(param.begin, box);
    if (begin_rect.empty()) {
        LogError << "failed to get swipe begin rect" << VAR(name);
        return { };
    }
    cv::Point begin = rand_point(begin_rect);

    std::vector<cv::Point> end;
    for (size_t i = 0; i < param.end.size(); ++i) {
        const auto& e = param.end.at(i);
        cv::Rect end_offset = param.end_offset.empty()      ? cv::Rect { }
                              : i < param.end_offset.size() ? param.end_offset.at(i)
                                                            : param.end_offset.back();
        MAA_RES_NS::Action::Target end_target { .type = e.type, .param = e.param, .offset = end_offset };
        auto end_rect = helper_.get_target_rect(end_target, box);
        if (end_rect.empty()) {
            LogError << "failed to get swipe end rect" << VAR(name) << VAR(i);
            return { };
        }
        end.emplace_back(rand_point(end_rect));
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
        return { };
    }

    std::vector<MAA_CTRL_NS::SwipeParam> swipes;
    for (const auto& swipe : param.swipes) {
        auto swipe_begin_rect = helper_.get_target_rect(swipe.begin, box);
        if (swipe_begin_rect.empty()) {
            LogError << "failed to get multi_swipe begin rect" << VAR(name);
            return { };
        }
        cv::Point begin = rand_point(swipe_begin_rect);

        std::vector<cv::Point> end;
        for (size_t i = 0; i < swipe.end.size(); ++i) {
            const auto& e = swipe.end.at(i);
            cv::Rect end_offset = swipe.end_offset.empty()      ? cv::Rect { }
                                  : i < swipe.end_offset.size() ? swipe.end_offset.at(i)
                                                                : swipe.end_offset.back();
            MAA_RES_NS::Action::Target end_target { .type = e.type, .param = e.param, .offset = end_offset };
            auto swipe_end_rect = helper_.get_target_rect(end_target, box);
            if (swipe_end_rect.empty()) {
                LogError << "failed to get multi_swipe end rect" << VAR(name) << VAR(i);
                return { };
            }
            end.emplace_back(rand_point(swipe_end_rect));
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
        return { };
    }

    auto target_rect = helper_.get_target_rect(param.target, box);
    if (target_rect.empty()) {
        LogError << "failed to get target rect" << VAR(name);
        return { };
    }
    cv::Point point = rand_point(target_rect);
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
        return { };
    }

    auto target_rect = helper_.get_target_rect(param.target, box);
    if (target_rect.empty()) {
        LogError << "failed to get target rect" << VAR(name);
        return { };
    }
    cv::Point point = rand_point(target_rect);
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
        return { };
    }

    MAA_CTRL_NS::TouchParam ctrl_param { .contact = static_cast<int>(param.contact), .point = { }, .pressure = 0 };
    bool ret = controller()->touch_up(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "TouchUp",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::click_key(const MAA_RES_NS::Action::ClickKeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = param.keys };
    bool ret = controller()->click_key(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "ClickKey",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::long_press_key(const MAA_RES_NS::Action::LongPressKeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::LongPressKeyParam ctrl_param { .keycode = param.keys, .duration = param.duration };
    bool ret = controller()->long_press_key(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "LongPressKey",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::key_down(const MAA_RES_NS::Action::KeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = { param.key } };
    bool ret = controller()->key_down(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "KeyDown",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::key_up(const MAA_RES_NS::Action::KeyParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::ClickKeyParam ctrl_param { .keycode = { param.key } };
    bool ret = controller()->key_up(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "KeyUp",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::input_text(const MAA_RES_NS::Action::InputTextParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::InputTextParam ctrl_param { .text = param.text };
    bool ret = controller()->input_text(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "InputText",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::scroll(const MAA_RES_NS::Action::ScrollParam& param, const cv::Rect& box, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    auto target_rect = helper_.get_target_rect(param.target, box);
    if (target_rect.empty()) {
        LogError << "failed to get target rect" << VAR(name);
        return { };
    }
    cv::Point point = rand_point(target_rect);
    MAA_CTRL_NS::ScrollParam ctrl_param { .point = point, .dx = param.dx, .dy = param.dy };
    bool ret = controller()->scroll(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Scroll",
        .box = box,
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::shell(const MAA_RES_NS::Action::ShellParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    std::string output;
    bool ret = controller()->shell(param.cmd, output, param.shell_timeout);

    LogDebug << "Shell command executed" << VAR(param.cmd) << VAR(param.shell_timeout) << VAR(ret);
    if (!output.empty()) {
        LogTrace << "Shell output:" << output;
    }

    json::object detail { { "cmd", param.cmd }, { "shell_timeout", param.shell_timeout }, { "success", ret }, { "output", output } };

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Shell",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(detail),
    };
}

ActionResult Actuator::screencap(const MAA_RES_NS::Action::ScreencapParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    auto image = controller()->cached_image();
    if (image.empty()) {
        LogError << "cached_image is empty";
        return { };
    }

    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    std::string ext = "." + param.format;
    std::string filename = param.filename.empty() ? std::format("{}_{}{}", format_now_for_filename(), name, ext) : param.filename + ext;
    auto filepath = std::filesystem::absolute(option.log_dir() / "screencap" / path(filename));

    std::vector<int> encode_params;
    if (param.format == "jpg" || param.format == "jpeg") {
        encode_params = { cv::IMWRITE_JPEG_QUALITY, param.quality };
    }

    bool ret = imwrite(filepath, image, encode_params);
    if (ret) {
        LogInfo << "screencap saved to" << filepath;
    }
    else {
        LogError << "failed to save screencap" << VAR(filepath) << VAR(param.format) << VAR(param.quality);
    }

    json::object detail {
        { "filepath", path_to_utf8_string(filepath) },
        { "format", param.format },
        { "quality", param.quality },
        { "success", ret },
    };

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "Screencap",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(detail),
    };
}

ActionResult Actuator::start_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::AppParam ctrl_param { .package = param.package };
    bool ret = controller()->start_app(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "StartApp",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult Actuator::stop_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }

    MAA_CTRL_NS::AppParam ctrl_param { .package = param.package };
    bool ret = controller()->stop_app(ctrl_param);

    return ActionResult {
        .action_id = action_id_,
        .name = name,
        .action = "StopApp",
        .box = cv::Rect { },
        .success = ret,
        .detail = json::value(ctrl_param),
    };
}

ActionResult
    Actuator::command(const MAA_RES_NS::Action::CommandParam& param, const cv::Rect& box, const std::string& name, const std::string& entry)
{
    if (!controller()) {
        LogError << "Controller is null";
        return { };
    }
    auto* resource = tasker_ ? tasker_->resource() : nullptr;
    if (!resource) {
        LogError << "Resource is null";
        return { };
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
        return { };
    }
    if (!tasker_->resource()) {
        LogError << "resource is null";
        return { };
    }
    auto session = tasker_->resource()->custom_action(param.name);
    auto rect = helper_.get_target_rect(param.target, box);
    if (rect.empty()) {
        LogWarn << "custom action target rect is empty, continue with empty rect" << VAR(name) << VAR(param.name);
    }
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
        .box = cv::Rect { },
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
        .box = cv::Rect { },
        .success = true,
        .detail = json::object(),
    };
}

MAA_CTRL_NS::ControllerAgent* Actuator::controller()
{
    return tasker_ ? tasker_->controller() : nullptr;
}

MAA_TASK_NS_END
