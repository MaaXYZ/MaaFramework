#include "CustomController.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_NS_BEGIN

CustomController::CustomController(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                                   MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), handle_(handle)
{}

std::string CustomController::get_uuid() const
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_uuid);

    if (!handle_ || !handle_->get_uuid) {
        LogError << "handle_ or handle_->get_uuid is nullptr";
        return {};
    }

    MaaSize buff_size = handle_->get_uuid(nullptr, 0);
    auto buff = std::make_unique<char[]>(buff_size);
    MaaSize written = handle_->get_uuid(buff.get(), buff_size);

    LogTrace << VAR(buff_size) << VAR(buff) << VAR(written);

    if (written != buff_size) {
        LogError << "written != buff_size";
        return {};
    }
    std::string uuid(buff.get(), buff.get() + written);
    LogTrace << VAR(uuid);
    return uuid;
}

bool CustomController::_connect()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->connect);

    if (!handle_ || !handle_->connect) {
        LogError << "handle_ or handle_->connect is nullptr";
        return false;
    }

    return handle_->connect();
}

std::pair<int, int> CustomController::_get_resolution() const
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_resolution);
    if (!handle_ || !handle_->get_resolution) {
        LogError << "handle_ or handle_->get_resolution is nullptr";
        return {};
    }
    int width = 0, height = 0;
    handle_->get_resolution(&width, &height);
    return { width, height };
}

void CustomController::_click(ClickParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->click) << VAR(param.x) << VAR(param.y);

    if (!handle_ || !handle_->click) {
        LogError << "handle_ or handle_->click is nullptr";
        return;
    }

    handle_->click(param.x, param.y);
}

void CustomController::_swipe(SwipeParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->swipe) << VAR(param.steps.size()) << VAR(param.steps.front())
            << VAR(param.steps.back());

    if (!handle_ || !handle_->swipe) {
        LogError << "handle_ or handle_->swipe is nullptr";
        return;
    }
    size_t size = param.steps.size();

    std::vector<int32_t> x_buf, y_buf, delay_buf;
    x_buf.reserve(size);
    y_buf.reserve(size);
    delay_buf.reserve(size);

    for (const auto& step : param.steps) {
        x_buf.emplace_back(step.x);
        y_buf.emplace_back(step.y);
        delay_buf.emplace_back(step.delay);
    }

    handle_->swipe(x_buf.data(), y_buf.data(), delay_buf.data(), size);
}

void CustomController::_press_key(PressKeyParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->press_key) << VAR(param.keycode);

    if (!handle_ || !handle_->press_key) {
        LogError << "handle_ or handle_->press_key is nullptr";
        return;
    }

    handle_->press_key(param.keycode);
}

cv::Mat CustomController::_screencap()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_image);

    if (!handle_ || !handle_->get_image) {
        LogError << "handle_ or handle_->get_image is nullptr";
        return {};
    }

    MaaSize size = handle_->get_image(nullptr, 0);
    if (size == 0 || size == MaaNullSize) {
        LogError << "error size" << VAR(size);
        return {};
    }

    auto buffer = std::make_unique<uchar[]>(size);
    MaaSize written = handle_->get_image(buffer.get(), size);
    if (written != size) {
        LogError << "written != size" << VAR(written) << VAR(size);
        return {};
    }

    std::vector vec_buf(buffer.get(), buffer.get() + written);
    cv::Mat res = cv::imdecode(vec_buf, cv::IMREAD_COLOR);
    if (res.empty()) {
        LogError << "image is empty!" << VAR(written) << VAR(size);
        return {};
    }
    return res;
}

bool CustomController::_start_app(AppParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->start_app) << VAR(param.package);

    if (!handle_ || !handle_->start_app) {
        LogError << "handle_ or handle_->start_app is nullptr";
        return false;
    }

    return handle_->start_app(param.package.c_str());
}

bool CustomController::_stop_app(AppParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->stop_app) << VAR(param.package);

    if (!handle_ || !handle_->stop_app) {
        LogError << "handle_ or handle_->stop_app is nullptr";
        return false;
    }

    return handle_->stop_app(param.package.c_str());
}

MAA_CTRL_NS_END
