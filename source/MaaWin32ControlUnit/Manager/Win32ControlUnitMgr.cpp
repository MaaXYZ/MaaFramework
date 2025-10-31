#include "Win32ControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Screencap/HwndUtils.hpp"


MAA_CTRL_UNIT_NS_BEGIN

Win32ControlUnitMgr::Win32ControlUnitMgr(HWND hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method)
    : hwnd_(hWnd)
    , screencap_method_(screencap_method)
    , input_method_(input_method)
{
}

bool Win32ControlUnitMgr::connect()
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

bool Win32ControlUnitMgr::request_uuid(std::string& uuid)
{
    if (!hwnd_) {
        LogWarn << "hwnd_ is nullptr";
    }

    std::stringstream ss;
    ss << hwnd_;
    uuid = std::move(ss).str();

    return true;
}

bool Win32ControlUnitMgr::start_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool Win32ControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool Win32ControlUnitMgr::screencap(cv::Mat& image)
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

bool Win32ControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click(x, y);
}

bool Win32ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool Win32ControlUnitMgr::is_touch_availabled() const
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->is_touch_availabled();
}

bool Win32ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_up(contact);
}

bool Win32ControlUnitMgr::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click_key(key);
}

bool Win32ControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->input_text(text);
}

bool Win32ControlUnitMgr::is_key_down_up_availabled() const
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->is_key_down_up_availabled();
}

bool Win32ControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->key_down(key);
}

bool Win32ControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->key_up(key);
}

MAA_CTRL_UNIT_NS_END
