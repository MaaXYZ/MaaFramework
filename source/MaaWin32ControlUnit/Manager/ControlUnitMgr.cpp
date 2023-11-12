#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitMgr::ControlUnitMgr(HWND hWnd, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : hWnd_(hWnd), notifier(callback, callback_arg)
{}

bool ControlUnitMgr::find_device(std::vector<std::string>& devices)
{
    std::ignore = devices;

    // TODO
    return true;
}

bool ControlUnitMgr::connect()
{
    if (!hWnd_) {
        LogError << "hWnd_ is nullptr";
        return false;
    }

    if (!IsWindow(hWnd_)) {
        LogError << "hWnd_ is invalid";
        return false;
    }

    if (IsIconic(hWnd_)) {
        LogError << "hWnd_ is minimized";
        return false;
    }

    return true;
}

bool ControlUnitMgr::request_uuid(std::string& uuid)
{
    if (!hWnd_) {
        LogError << "hWnd_ is nullptr";
        return false;
    }

    uuid = std::to_string(reinterpret_cast<uintptr_t>(hWnd_));
    return true;
}

bool ControlUnitMgr::request_resolution(int& width, int& height)
{
    if (!hWnd_) {
        LogError << "hWnd_ is nullptr";
        return false;
    }

    RECT rect;
    if (!GetClientRect(hWnd_, &rect)) {
        LogError << "failed to GetClientRect";
        return false;
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

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
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->click(x, y);
}

bool ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->swipe(x1, y1, x2, y2, duration);
}

bool ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_down(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_move(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_up(int contact)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_up(contact);
}

bool ControlUnitMgr::press_key(int key)
{
    if (!key_input_) {
        LogError << "key_input_ is null";
        return false;
    }

    return key_input_->press_key(key);
}

MAA_CTRL_UNIT_NS_END
