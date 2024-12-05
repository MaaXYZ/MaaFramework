#include "ControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Screencap/HwndUtils.hpp"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitMgr::ControlUnitMgr(HWND hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method)
    : hwnd_(hWnd)
    , screencap_method_(screencap_method)
    , input_method_(input_method)
{
}

bool ControlUnitMgr::find_device(std::vector<std::string>& devices)
{
    std::ignore = devices;

    // TODO
    return true;
}

bool ControlUnitMgr::connect()
{
    if (hwnd_) {
        if (!IsWindow(hwnd_)) {
            LogError << "hwnd_ is invalid";
            return false;
        }

        if (IsIconic(hwnd_)) {
            LogError << "hwnd_ is minimized";
            return false;
        }
    }
    else {
        LogWarn << "hwnd_ is nullptr";
    }

    if (screencap_method_ != MaaWin32ScreencapMethod_None) {
        screencap_ = std::make_shared<ScreencapAgent>(screencap_method_, hwnd_);
    }
    else {
        LogWarn << "screencap_method_ is None";
    }

    if (input_method_ != MaaWin32InputMethod_None) {
        input_ = std::make_shared<InputAgent>(input_method_, hwnd_);
    }
    else {
        LogWarn << "input_method_ is None";
    }

    return true;
}

bool ControlUnitMgr::request_uuid(std::string& uuid)
{
    if (!hwnd_) {
        LogWarn << "hwnd_ is nullptr";
    }

    std::stringstream ss;
    ss << hwnd_;
    uuid = std::move(ss).str();

    return true;
}

bool ControlUnitMgr::start_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool ControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool ControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is null";
        return false;
    }

    auto opt = screencap_->screencap();
    if (!opt) {
        LogError << "failed to screencap";
        return false;
    }

    image = std::move(opt).value();

    return true;
}

bool ControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click(x, y);
}

bool ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool ControlUnitMgr::multi_swipe(const std::vector<SwipeParam>& swipes)
{
    std::ignore = swipes;

    LogError << "Win32 not support MultiSwipe. If you need it, please raise an issue with us!" << VAR(swipes.size());

    return false;
}

bool ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_up(contact);
}

bool ControlUnitMgr::press_key(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->press_key(key);
}

bool ControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->input_text(text);
}

MAA_CTRL_UNIT_NS_END
