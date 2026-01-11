#include "ControllerAgent.h"

#include "Global/OptionMgr.h"
#include "Global/PluginMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Resource/ResourceMgr.h"

MAA_CTRL_NS_BEGIN

ControllerAgent::ControllerAgent(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit)
    : control_unit_(std::move(control_unit))
{
    LogFunc << VAR(control_unit_);

    auto& plugin_mgr = MAA_GLOBAL_NS::PluginMgr::get_instance();
    LogInfo << VAR(plugin_mgr.get_names());

    for (const auto& sink : plugin_mgr.get_ctrl_sinks()) {
        add_sink(sink, this);
    }

    action_runner_ =
        std::make_unique<AsyncRunner<Action>>(std::bind(&ControllerAgent::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerAgent::~ControllerAgent()
{
    LogFunc;

    if (action_runner_) {
        action_runner_->wait_all();
    }
}

bool ControllerAgent::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogInfo << VAR(key) << VAR(value) << VAR(val_size);

    switch (key) {
    case MaaCtrlOption_ScreenshotTargetLongSide:
        return set_image_target_long_side(value, val_size);
    case MaaCtrlOption_ScreenshotTargetShortSide:
        return set_image_target_short_side(value, val_size);
    case MaaCtrlOption_ScreenshotUseRawSize:
        return set_image_use_raw_size(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaCtrlId ControllerAgent::post_connection()
{
    auto id = post({ .type = Action::Type::connect });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_click(int x, int y)
{
    ClickParam p { .point = { x, y } };
    auto id = post({ .type = Action::Type::click, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_swipe(int x1, int y1, int x2, int y2, int duration)
{
    SwipeParam p { .begin = { x1, y1 }, .end = { { x2, y2 } }, .duration = { static_cast<uint>(duration) } };
    auto id = post({ .type = Action::Type::swipe, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_click_key(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    auto id = post({ .type = Action::Type::click_key, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_input_text(const std::string& text)
{
    InputTextParam p { .text = text };
    auto id = post({ .type = Action::Type::input_text, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_start_app(const std::string& intent)
{
    AppParam p { .package = intent };
    auto id = post({ .type = Action::Type::start_app, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_stop_app(const std::string& intent)
{
    AppParam p { .package = intent };
    auto id = post({ .type = Action::Type::stop_app, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_screencap()
{
    auto id = post({ .type = Action::Type::screencap });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_touch_down(int contact, int x, int y, int pressure)
{
    TouchParam p { .contact = contact, .point = { x, y }, .pressure = pressure };
    auto id = post({ .type = Action::Type::touch_down, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_touch_move(int contact, int x, int y, int pressure)
{
    TouchParam p { .contact = contact, .point = { x, y }, .pressure = pressure };
    auto id = post({ .type = Action::Type::touch_move, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_touch_up(int contact)
{
    TouchParam p { .contact = contact };
    auto id = post({ .type = Action::Type::touch_up, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_key_down(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    auto id = post({ .type = Action::Type::key_down, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_key_up(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    auto id = post({ .type = Action::Type::key_up, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_scroll(int dx, int dy)
{
    ScrollParam p { .dx = dx, .dy = dy };
    auto id = post({ .type = Action::Type::scroll, .param = std::move(p) });
    return focus_id(id);
}

MaaCtrlId ControllerAgent::post_shell(const std::string& cmd, int64_t timeout)
{
    ShellParam p { .cmd = cmd, .timeout = timeout };
    auto id = post({ .type = Action::Type::shell, .param = std::move(p) });
    return focus_id(id);
}

MaaStatus ControllerAgent::status(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(action_runner_->status(ctrl_id));
}

MaaStatus ControllerAgent::wait(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    if (ctrl_id == MaaInvalidId) {
        return MaaStatus_Invalid;
    }

    action_runner_->wait(ctrl_id);
    return status(ctrl_id);
}

bool ControllerAgent::connected() const
{
    return connected_;
}

cv::Mat ControllerAgent::cached_image() const
{
    std::unique_lock lock(image_mutex_);
    return image_.clone();
}

std::string ControllerAgent::cached_shell_output() const
{
    std::unique_lock lock(shell_output_mutex_);
    return shell_output_;
}

std::string ControllerAgent::get_uuid()
{
    if (uuid_cache_.empty()) {
        request_uuid();
    }
    return uuid_cache_;
}

MaaSinkId ControllerAgent::add_sink(MaaEventCallback callback, void* trans_arg)
{
    return notifier_.add_sink(callback, trans_arg);
}

void ControllerAgent::remove_sink(MaaSinkId sink_id)
{
    notifier_.remove_sink(sink_id);
}

void ControllerAgent::clear_sinks()
{
    notifier_.clear_sinks();
}

void ControllerAgent::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (action_runner_ && action_runner_->running()) {
        action_runner_->clear();
    }
}

bool ControllerAgent::running() const
{
    return action_runner_ && action_runner_->running();
}

bool ControllerAgent::click(ClickParam p)
{
    auto id = post({ .type = Action::Type::click, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::long_press(LongPressParam p)
{
    auto id = post({ .type = Action::Type::long_press, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::swipe(SwipeParam p)
{
    auto id = post({ .type = Action::Type::swipe, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::multi_swipe(MultiSwipeParam p)
{
    auto id = post({ .type = Action::Type::multi_swipe, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_down(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_down, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_move(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_move, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_up(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_up, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::click_key(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::click_key, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::long_press_key(LongPressKeyParam p)
{
    auto id = post({ .type = Action::Type::long_press_key, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::key_down(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::key_down, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::key_up(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::key_up, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::input_text(InputTextParam p)
{
    auto id = post({ .type = Action::Type::input_text, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

cv::Mat ControllerAgent::screencap()
{
    auto id = post({ .type = Action::Type::screencap });
    if (wait(id) != MaaStatus_Succeeded) {
        return {};
    }
    return cached_image();
}

bool ControllerAgent::start_app(AppParam p)
{
    auto id = post({ .type = Action::Type::start_app, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::stop_app(AppParam p)
{
    auto id = post({ .type = Action::Type::stop_app, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::scroll(ScrollParam p)
{
    auto id = post({ .type = Action::Type::scroll, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::shell(const std::string& cmd, std::string& output, int64_t timeout)
{
    ShellParam p { .cmd = cmd, .timeout = timeout };
    auto id = post({ .type = Action::Type::shell, .param = std::move(p) });
    bool ret = wait(id) == MaaStatus_Succeeded;
    if (ret) {
        output = cached_shell_output();
    }
    return ret;
}

MaaCtrlId ControllerAgent::post(Action action)
{
    // LogInfo << VAR(action.type) << VAR(action.param);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    if (!action_runner_) {
        return MaaInvalidId;
    }
    return action_runner_->post(std::move(action));
}

MaaCtrlId ControllerAgent::focus_id(MaaCtrlId id)
{
    if (id == MaaInvalidId) {
        return id;
    }

    std::unique_lock lock { focus_ids_mutex_ };
    return *focus_ids_.emplace(id).first;
}

bool ControllerAgent::handle_connect()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    connected_ = control_unit_->connect();

    request_uuid();

    return connected_;
}

bool ControllerAgent::handle_click(const ClickParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);

    bool ret = true;
    if (control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick) {
        ret &= control_unit_->touch_down(param.contact, point.x, point.y, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ret &= control_unit_->touch_up(param.contact);
    }
    else {
        ret &= control_unit_->click(point.x, point.y);
    }

    return ret;
}

bool ControllerAgent::handle_long_press(const LongPressParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);

    bool ret = true;
    if (control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick) {
        ret &= control_unit_->touch_down(param.contact, point.x, point.y, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(param.duration));
        ret &= control_unit_->touch_up(param.contact);
    }
    else {
        LogWarn << "long press not supported, use click instead";
        ret &= control_unit_->click(point.x, point.y);
    }

    return ret;
}

bool ControllerAgent::handle_swipe(const SwipeParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    const bool use_touch_down_up = control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    if (!use_touch_down_up) {
        LogWarn << "touch not supported, use swipe instead. some features can not work";
    }

    cv::Point begin = preproc_touch_point(param.begin);
    bool ret = !param.end.empty();

    if (!param.only_hover && use_touch_down_up) {
        ret &= control_unit_->touch_down(param.contact, begin.x, begin.y, 1);
    }

    for (size_t i = 0; i < param.end.size(); ++i) {
        const cv::Point& end = preproc_touch_point(param.end.at(i));
        const uint duration = param.duration.empty() ? 200 : (i < param.duration.size()) ? param.duration.at(i) : param.duration.back();
        const uint end_hold = param.end_hold.empty() ? 0 : (i < param.end_hold.size()) ? param.end_hold.at(i) : param.end_hold.back();

        if (use_touch_down_up) {
            constexpr double kInterval = 10; // ms
            const std::chrono::milliseconds delay(static_cast<int>(kInterval));

            const double total_step = duration / kInterval;
            const double x_step_len = (end.x - begin.x) / total_step;
            const double y_step_len = (end.y - begin.y) / total_step;

            auto now = std::chrono::steady_clock::now();

            for (int step = 1; step < total_step; ++step) {
                int mx = static_cast<int>(begin.x + step * x_step_len);
                int my = static_cast<int>(begin.y + step * y_step_len);

                std::this_thread::sleep_until(now + delay);

                now = std::chrono::steady_clock::now();
                ret &= control_unit_->touch_move(param.contact, mx, my, 1);
            }

            std::this_thread::sleep_until(now + delay);

            now = std::chrono::steady_clock::now();
            ret &= control_unit_->touch_move(param.contact, end.x, end.y, 1);

            std::this_thread::sleep_until(now + delay);
        }
        else {
            ret &= control_unit_->swipe(begin.x, begin.y, end.x, end.y, duration);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(end_hold));

        begin = end;
    }

    if (!param.only_hover && use_touch_down_up) {
        ret &= control_unit_->touch_up(param.contact);
    }

    return ret;
}

bool ControllerAgent::handle_multi_swipe(const MultiSwipeParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    if (!(control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick)) {
        LogError << "touch is not available";
        return false;
    }

    constexpr double kInterval = 10; // ms
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    struct SegmentOperating
    {
        cv::Point begin {};
        cv::Point end {};
        double total_step = 0;
        double x_step_len = 0;
        double y_step_len = 0;
        size_t step_index = 0;
        uint duration = 0;
        uint end_hold = 0;
    };

    struct CotactOperating
    {
        std::vector<SegmentOperating> seg;
        size_t seg_index = 0;
    };

    // contact index < end index < op >>
    std::vector<CotactOperating> operating(param.swipes.size());

    for (size_t s_i = 0; s_i < param.swipes.size(); ++s_i) {
        const SwipeParam& s = param.swipes.at(s_i);

        cv::Point begin = preproc_touch_point(s.begin);
        for (size_t e_i = 0; e_i < s.end.size(); ++e_i) {
            const cv::Point& end = preproc_touch_point(s.end.at(e_i));
            const uint duration = s.duration.empty() ? 200 : (e_i < s.duration.size()) ? s.duration.at(e_i) : s.duration.back();
            const uint end_hold = s.end_hold.empty() ? 0 : (e_i < s.end_hold.size()) ? s.end_hold.at(e_i) : s.end_hold.back();

            SegmentOperating o;
            o.begin = begin;
            o.end = end;
            o.total_step = duration / kInterval;
            o.x_step_len = (end.x - begin.x) / o.total_step;
            o.y_step_len = (end.y - begin.y) / o.total_step;
            o.step_index = 0;
            o.duration = duration;
            o.end_hold = end_hold;

            operating.at(s_i).seg.emplace_back(std::move(o));
            operating.at(s_i).seg_index = 0;

            begin = end;
        }
    }

    const auto starting = std::chrono::steady_clock::now();
    auto now = starting;

    bool ret = !param.swipes.empty();
    size_t over_count = 0;
    while (over_count < param.swipes.size()) {
        uint now_point = static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(now - starting).count());

        for (size_t i = 0; i < param.swipes.size(); ++i) {
            const SwipeParam& s = param.swipes.at(i);
            if (now_point < s.starting) {
                continue;
            }

            int contact = s.contact != 0 ? s.contact : static_cast<int>(i);

            if (i >= operating.size()) {
                LogError << "Invalid contact index" << VAR(i) << VAR(operating.size());
                continue;
            }
            auto& contact_op = operating.at(i);

            if (contact_op.seg_index >= contact_op.seg.size()) {
                continue; // all segments done
            }
            auto& seg_op = contact_op.seg.at(contact_op.seg_index);

            if (seg_op.step_index == 0) {
                if (!s.only_hover) {
                    ret &= control_unit_->touch_down(contact, seg_op.begin.x, seg_op.begin.y, 1);
                }
                ++seg_op.step_index;
            }
            else if (seg_op.step_index < seg_op.total_step) {
                int mx = static_cast<int>(seg_op.begin.x + seg_op.step_index * seg_op.x_step_len);
                int my = static_cast<int>(seg_op.begin.y + seg_op.step_index * seg_op.y_step_len);
                ret &= control_unit_->touch_move(contact, mx, my, 1);
                ++seg_op.step_index;
            }
            else if (seg_op.step_index == seg_op.total_step) {
                if (!s.only_hover) {
                    ret &= control_unit_->touch_up(contact);
                }
                ++seg_op.step_index;
                ++over_count;
            }
            else { // step_index > total
                ++contact_op.seg_index;
                continue;
            }
        }

        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();
    }

    return ret;
}

bool ControllerAgent::handle_touch_down(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);
    bool ret = control_unit_->touch_down(param.contact, point.x, point.y, param.pressure);

    return ret;
}

bool ControllerAgent::handle_touch_move(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);
    bool ret = control_unit_->touch_move(param.contact, point.x, point.y, param.pressure);

    return ret;
}

bool ControllerAgent::handle_touch_up(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->touch_up(param.contact);

    return ret;
}

bool ControllerAgent::handle_click_key(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    bool use_key_down_up = control_unit_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;

    for (const auto& keycode : param.keycode) {
        if (use_key_down_up) {
            ret &= control_unit_->key_down(keycode);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ret &= control_unit_->key_up(keycode);
        }
        else {
            ret &= control_unit_->click_key(keycode);
        }
    }

    return ret;
}

bool ControllerAgent::handle_long_press_key(const LongPressKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    bool use_key_down_up = control_unit_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;

    for (const auto& keycode : param.keycode) {
        if (use_key_down_up) {
            ret &= control_unit_->key_down(keycode);
            std::this_thread::sleep_for(std::chrono::milliseconds(param.duration));
            ret &= control_unit_->key_up(keycode);
        }
        else {
            LogWarn << "long press key not supported, use click instead";
            ret &= control_unit_->click_key(keycode);
        }
    }

    return ret;
}

bool ControllerAgent::handle_input_text(const InputTextParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->input_text(param.text);

    return ret;
}

bool ControllerAgent::handle_screencap()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Mat raw_image;
    bool screencaped = control_unit_->screencap(raw_image);
    if (!screencaped) {
        LogError << "controller screencap failed";
        return false;
    }

    bool ret = postproc_screenshot(raw_image);

    return ret;
}

bool ControllerAgent::handle_start_app(const AppParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->start_app(param.package);

    return ret;
}

bool ControllerAgent::handle_stop_app(const AppParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->stop_app(param.package);

    return ret;
}

bool ControllerAgent::handle_key_down(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    for (const auto& keycode : param.keycode) {
        ret &= control_unit_->key_down(keycode);
    }

    return ret;
}

bool ControllerAgent::handle_key_up(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    for (const auto& keycode : param.keycode) {
        ret &= control_unit_->key_up(keycode);
    }

    return ret;
}

bool ControllerAgent::handle_scroll(const ScrollParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->scroll(param.dx, param.dy);
    return ret;
}

bool ControllerAgent::handle_shell(const ShellParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    auto adb_unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::AdbControlUnitAPI>(control_unit_);
    if (!adb_unit) {
        LogError << "Shell commands are only supported for ADB controllers. Current controller type does not support shell execution.";
        return false;
    }

    std::string output;
    bool ret = adb_unit->shell(param.cmd, output, std::chrono::milliseconds(param.timeout));
    if (ret) {
        std::unique_lock lock(shell_output_mutex_);
        shell_output_ = std::move(output);
    }

    return ret;
}

bool ControllerAgent::check_stop()
{
    if (!need_to_stop_) {
        return true;
    }

    if (running()) {
        LogError << "stopping, ignore new post";
        return false;
    }

    need_to_stop_ = false;
    return true;
}

bool ControllerAgent::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    bool ret = false;

    bool notify = false;
    {
        std::unique_lock lock { focus_ids_mutex_ };
        notify = focus_ids_.erase(id) > 0;
    }

    const json::value cb_detail = {
        { "ctrl_id", id },
        { "uuid", get_uuid() },
        { "action", action.type },
        { "param", action.param },
    };

    // LogInfo << cb_detail.to_string();

    if (notify) {
        notifier_.notify(this, MaaMsg_Controller_Action_Starting, cb_detail);
    }

    switch (action.type) {
    case Action::Type::connect:
        ret = handle_connect();
        break;

    case Action::Type::click:
        ret = handle_click(std::get<ClickParam>(action.param));
        break;
    case Action::Type::long_press:
        ret = handle_long_press(std::get<LongPressParam>(action.param));
        break;
    case Action::Type::swipe:
        ret = handle_swipe(std::get<SwipeParam>(action.param));
        break;
    case Action::Type::multi_swipe:
        ret = handle_multi_swipe(std::get<MultiSwipeParam>(action.param));
        break;

    case Action::Type::touch_down:
        ret = handle_touch_down(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_move:
        ret = handle_touch_move(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_up:
        ret = handle_touch_up(std::get<TouchParam>(action.param));
        break;

    case Action::Type::click_key:
        ret = handle_click_key(std::get<ClickKeyParam>(action.param));
        break;
    case Action::Type::long_press_key:
        ret = handle_long_press_key(std::get<LongPressKeyParam>(action.param));
        break;
    case Action::Type::input_text:
        ret = handle_input_text(std::get<InputTextParam>(action.param));
        break;

    case Action::Type::key_down:
        ret = handle_key_down(std::get<ClickKeyParam>(action.param));
        break;

    case Action::Type::key_up:
        ret = handle_key_up(std::get<ClickKeyParam>(action.param));
        break;

    case Action::Type::screencap:
        ret = handle_screencap();
        break;

    case Action::Type::start_app:
        ret = handle_start_app(std::get<AppParam>(action.param));
        break;
    case Action::Type::stop_app:
        ret = handle_stop_app(std::get<AppParam>(action.param));
        break;

    case Action::Type::scroll:
        ret = handle_scroll(std::get<ScrollParam>(action.param));
        break;

    case Action::Type::shell:
        ret = handle_shell(std::get<ShellParam>(action.param));
        break;

    default:
        LogError << "Unknown action type" << VAR(static_cast<int>(action.type));
        ret = false;
    }

    if (ret && notify) {
        notifier_.notify(this, MaaMsg_Controller_Action_Succeeded, cb_detail);
    }
    else if (!ret) {
        notifier_.notify(this, MaaMsg_Controller_Action_Failed, cb_detail);
    }

    return ret;
}

cv::Point ControllerAgent::preproc_touch_point(const cv::Point& p)
{
    if (image_target_width_ == 0 || image_target_height_ == 0) {
        LogWarn << "Invalid image target size" << VAR(image_target_width_) << VAR(image_target_height_);

        if (!init_scale_info()) {
            return {};
        }
    }

    double scale_width = static_cast<double>(image_raw_width_) / image_target_width_;
    double scale_height = static_cast<double>(image_raw_height_) / image_target_height_;

    int proced_x = static_cast<int>(std::round(p.x * scale_width));
    int proced_y = static_cast<int>(std::round(p.y * scale_height));

    return { proced_x, proced_y };
}

bool ControllerAgent::postproc_screenshot(const cv::Mat& raw)
{
    if (raw.empty()) {
        image_ = cv::Mat();
        LogError << "Empty screenshot";
        return false;
    }

    if (raw.cols != image_raw_width_ || raw.rows != image_raw_height_ || image_target_width_ == 0 || image_target_height_ == 0) {
        LogInfo << "Resolution changed" << VAR(raw.cols) << VAR(raw.rows) << VAR(image_raw_width_) << VAR(image_raw_height_);

        image_raw_width_ = raw.cols;
        image_raw_height_ = raw.rows;

        if (!calc_target_image_size()) {
            image_ = cv::Mat();
            LogError << "Invalid target image size";
            return false;
        }
    }

    cv::resize(raw, image_, { image_target_width_, image_target_height_ }, 0, 0, cv::INTER_AREA);
    return !image_.empty();
}

bool ControllerAgent::calc_target_image_size()
{
    if (image_target_long_side_ == 0 && image_target_short_side_ == 0) {
        LogError << "Invalid image target size";
        return false;
    }

    if (image_use_raw_size_) {
        LogDebug << "image_use_raw_size_" << VAR(image_raw_width_) << VAR(image_raw_height_);
        image_target_width_ = image_raw_width_;
        image_target_height_ = image_raw_height_;
        return true;
    }

    LogDebug << "Re-calc image target size:" << VAR(image_target_long_side_) << VAR(image_target_short_side_) << VAR(image_raw_width_)
             << VAR(image_raw_height_);

    double scale = static_cast<double>(image_raw_width_) / image_raw_height_;

    if (image_target_short_side_ != 0) {
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = static_cast<int>(std::round(image_target_short_side_ * scale));
            image_target_height_ = image_target_short_side_;
        }
        else {
            image_target_width_ = image_target_short_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_short_side_ / scale));
        }
    }
    else { // image_target_long_side_ != 0
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = image_target_long_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_long_side_ / scale));
        }
        else {
            image_target_width_ = static_cast<int>(std::round(image_target_long_side_ * scale));
            image_target_height_ = image_target_long_side_;
        }
    }

    LogInfo << VAR(image_target_width_) << VAR(image_target_height_);
    return true;
}

void ControllerAgent::clear_target_image_size()
{
    LogDebug;

    image_target_width_ = 0;
    image_target_height_ = 0;
}

bool ControllerAgent::request_uuid()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    uuid_cache_.clear();

    return control_unit_->request_uuid(uuid_cache_);
}

bool ControllerAgent::init_scale_info()
{
    // 实际是通过 postproc_screenshot 初始化的
    return handle_screencap();
}

bool ControllerAgent::set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(image_target_long_side_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_long_side_ = *reinterpret_cast<int*>(value);
    image_target_short_side_ = 0;

    clear_target_image_size();

    LogInfo << "image_target_width_ = " << image_target_long_side_;
    return true;
}

bool ControllerAgent::set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(image_target_short_side_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_long_side_ = 0;
    image_target_short_side_ = *reinterpret_cast<int*>(value);

    clear_target_image_size();

    LogInfo << "image_target_height_ = " << image_target_short_side_;
    return true;
}

bool ControllerAgent::set_image_use_raw_size(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(bool)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_use_raw_size_ = *reinterpret_cast<bool*>(value);

    clear_target_image_size();

    return true;
}

MAA_CTRL_NS_END
