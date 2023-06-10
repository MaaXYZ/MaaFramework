#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

std::minstd_rand ControllerMgr::rand_engine_(std::random_device {}());

ControllerMgr::ControllerMgr(MaaControllerCallback callback, void* callback_arg) : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    action_runner_ = std::make_unique<AsyncRunner<Action>>(
        std::bind(&ControllerMgr::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerMgr::~ControllerMgr()
{
    LogFunc;

    if (action_runner_) {
        action_runner_->release();
    }

    notifier.release();
}

bool ControllerMgr::set_option(MaaCtrlOption key, std::string_view value)
{
    return false;
}

MaaCtrlId ControllerMgr::post_connection()
{
    return action_runner_->post({ .type = Action::Type::connect });
}

MaaCtrlId ControllerMgr::post_click(int x, int y)
{
    ClickParams params { .x = x, .y = y };
    return action_runner_->post({ .type = Action::Type::click, .params = std::move(params) });
}

MaaCtrlId ControllerMgr::post_swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay)
{
    SwipeParams params;
    for (size_t i = 0; i != x_steps.size(); ++i) {
        SwipeParams::Step step {
            .x = x_steps[i],
            .y = y_steps[i],
            .delay = step_delay[i],
        };
        params.steps.emplace_back(std::move(step));
    }

    return action_runner_->post({ .type = Action::Type::swipe, .params = std::move(params) });
}

MaaCtrlId ControllerMgr::post_screencap()
{
    return action_runner_->post({ .type = Action::Type::screencap });
}

MaaStatus ControllerMgr::status(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    return action_runner_->status(ctrl_id);
}

MaaStatus ControllerMgr::wait(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    action_runner_->wait(ctrl_id);
    return action_runner_->status(ctrl_id);
}

MaaBool ControllerMgr::connected() const
{
    return MaaBool();
}

std::vector<uint8_t> ControllerMgr::get_image() const
{
    return std::vector<uint8_t>();
}

std::string ControllerMgr::get_uuid() const
{
    return std::string();
}

void ControllerMgr::click(const cv::Rect& r)
{
    click(rand_point(r));
}

void ControllerMgr::click(const cv::Point& p)
{
    ClickParams params { .x = p.x, .y = p.y };
    action_runner_->post({ .type = Action::Type::click, .params = std::move(params) }, true);
}

void ControllerMgr::swipe(const cv::Rect& r1, const cv::Rect& r2, int duration)
{
    swipe(rand_point(r1), rand_point(r2), duration);
}

void ControllerMgr::swipe(const cv::Point& p1, const cv::Point& p2, int duration)
{
    // TODO: post swipe
}

cv::Mat ControllerMgr::screencap()
{
    std::unique_lock<std::mutex> lock(image_mutex_);
    action_runner_->post({ .type = Action::Type::screencap }, true);
    return image_;
}

bool ControllerMgr::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    LogFunc << VAR(id) << VAR(action);

    switch (action.type) {
    case Action::Type::connect:
        connected_ = _connect();
        return connected_;
    case Action::Type::click:
        _click(std::get<ClickParams>(action.params));
        return true;
    case Action::Type::swipe:
        _swipe(std::get<SwipeParams>(action.params));
        return true;
    case Action::Type::screencap:
        image_ = _screencap();
        return !image_.empty();
    }

    return false;
}

cv::Point ControllerMgr::rand_point(const cv::Rect& r)
{
    int x = 0, y = 0;

    if (r.width == 0) {
        x = r.x;
    }
    else {
        int x_rand = std::poisson_distribution<int>(r.width / 2.)(rand_engine_);
        x = x_rand + r.x;
    }

    if (r.height == 0) {
        y = r.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(r.height / 2.)(rand_engine_);
        y = y_rand + r.y;
    }

    return { x, y };
}

inline std::ostream& operator<<(std::ostream& os, const Action& action)
{
    switch (action.type) {
    case Action::Type::connect:
        os << "connect";
        break;
    case Action::Type::click:
        os << "click";
        break;
    case Action::Type::swipe:
        os << "swipe";
        break;
    case Action::Type::screencap:
        os << "screencap";
        break;
    }
    return os;
}

MAA_CTRL_NS_END
