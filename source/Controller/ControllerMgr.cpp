#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

MaaCtrlId ControllerMgr::connect(std::string adb_path, std::string address, std::string config)
{
    return runner_->call(CallItem { .type = CallItem::Connect,
                                    .param = CallItem::ConnectParams { .adb_path = std::move(adb_path),
                                                                       .address = std::move(address),
                                                                       .config = std::move(config) } });
}

MaaCtrlId ControllerMgr::click(int x, int y)
{
    return runner_->call(CallItem { .type = CallItem::Click, .param = CallItem::ClickParams { .x = x, .y = y } });
}

MaaCtrlId ControllerMgr::swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay)
{
    return runner_->call(CallItem { .type = CallItem::Swipe,
                                    .param = CallItem::SwipeParams { .x_steps = std::move(x_steps),
                                                                     .y_steps = std::move(y_steps),
                                                                     .step_delay = std::move(step_delay) } });
}

MaaCtrlId ControllerMgr::screencap()
{
    return runner_->call(CallItem { .type = CallItem::Screencap, .param = CallItem::ScreencapParams {} });
}

ControllerMgr::ControllerMgr(MaaControllerCallback callback, void* callback_arg)
    : callback_(callback), callback_arg_(callback_arg)
{
    runner_ =
        new AsyncRunner<CallItem>([this](auto id, CallItem item) { this->runnerDispatcher(id, std::move(item)); });
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

std::vector<unsigned char> ControllerMgr::get_image() const
{
    return std::vector<unsigned char>();
}

std::string ControllerMgr::get_uuid() const
{
    return std::string();
}

bool ControllerMgr::runnerDispatcher(MaaCtrlId id, CallItem item)
{
    // TODO: 任务计时

    bool result;

    switch (item.type) {
    case CallItem::Connect: {
        const auto& [adb_path, address, config] = std::get<CallItem::ConnectParams>(item.param);
        result = this->do_connect(adb_path, address, config);
        break;
    }
    case CallItem::Click: {
        const auto& [x, y] = std::get<CallItem::ClickParams>(item.param);
        result = this->do_click(x, y);
        break
    }
    case CallItem::Swipe: {
        const auto& [x_steps, y_steps, step_delay] = std::get<CallItem::SwipeParams>(item.param);
        result = this->do_swipe(x_steps, y_steps, step_delay);
        break;
    }
    case CallItem::Screencap: {
        std::unique_lock<std::mutex> lock(cache_image_mutex_);
        result = this->do_screencap(cache_image_);
        break;
    }
    }

    return result;
}

MAA_CTRL_NS_END
