#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

ControllerMgr::ControllerMgr(MaaControllerCallback callback, void* callback_arg) : notifier(callback, callback_arg)
{
    action_runner_ = std::make_unique<AsyncRunner<Action>>(
        std::bind(&ControllerMgr::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerMgr::~ControllerMgr() {}

bool ControllerMgr::set_option(std::string_view key, std::string_view value)
{
    return false;
}

bool ControllerMgr::connecting() const
{
    return false;
}

bool ControllerMgr::connected() const
{
    return false;
}

MaaCtrlId ControllerMgr::post_click(int x, int y)
{
    ClickParams params { .x = x, .y = y };
    return action_runner_->post({ .type = Action::Type::click, .params = std::move(params) });
}

MaaCtrlId ControllerMgr::post_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                                    const std::vector<int>& step_delay)
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

std::vector<unsigned char> ControllerMgr::get_image() const
{
    return std::vector<unsigned char>();
}

std::string ControllerMgr::get_uuid() const
{
    return std::string();
}

void ControllerMgr::click(const cv::Rect& r)
{
    // TODO: invoke _click
}

void ControllerMgr::click(const cv::Point& p)
{
    // TODO: invoke _click
}

void ControllerMgr::swipe(const cv::Rect& r1, const cv::Rect& r2, int duration)
{
    // TODO: invoke _swipe
}

void ControllerMgr::swipe(const cv::Point& p1, const cv::Point& p2, int duration)
{
    // TODO: invoke _swipe
}

cv::Mat ControllerMgr::screencap()
{
    // TODO: invoke _screencap
    return cv::Mat();
}

void ControllerMgr::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    switch (action.type) {
    case Action::Type::connect:
        _connect();
        break;
    case Action::Type::click:
        _click(std::get<ClickParams>(action.params));
        break;
    case Action::Type::swipe:
        _swipe(std::get<SwipeParams>(action.params));
        break;
    case Action::Type::screencap:
        _screencap();
        break;
    }
}

MAA_CTRL_NS_END
