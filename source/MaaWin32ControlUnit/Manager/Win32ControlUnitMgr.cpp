#include "Win32ControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"

#include "Input/GamepadInput.h"
#include "Input/LegacyEventInput.h"
#include "Input/MessageInput.h"
#include "Input/PostThreadMessageInput.h"
#include "Input/SeizeInput.h"
#include "Screencap/DesktopDupScreencap.h"
#include "Screencap/DesktopDupWindowScreencap.h"
#include "Screencap/FramePoolScreencap.h"
#include "Screencap/GdiScreencap.h"
#include "Screencap/HwndUtils.hpp"
#include "Screencap/PrintWindowScreencap.h"
#include "Screencap/ScreenDCScreencap.h"

MAA_CTRL_UNIT_NS_BEGIN

Win32ControlUnitMgr::Win32ControlUnitMgr(
    HWND hWnd,
    MaaWin32ScreencapMethod screencap_method,
    MaaWin32InputMethod mouse_method,
    MaaWin32InputMethod keyboard_method)
    : hwnd_(hWnd)
    , screencap_method_(screencap_method)
    , mouse_method_(mouse_method)
    , keyboard_method_(keyboard_method)
{
}

bool Win32ControlUnitMgr::connect()
{
#ifndef MAA_WIN32_COMPATIBLE
    // 设置 Per-Monitor DPI Aware V2，确保 GetClientRect/GetWindowRect 等 API 返回物理像素。
    // 修复高 DPI 缩放下 PrintWindow/FramePool 等截图方式只能截取部分区域的问题。
    auto prev_ctx = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (prev_ctx) {
        LogInfo << "SetThreadDpiAwarenessContext to PER_MONITOR_AWARE_V2 success" << VAR_VOIDP(prev_ctx);
    }
    else {
        LogWarn << "SetThreadDpiAwarenessContext failed, error:" << GetLastError();
    }
#endif

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

    switch (screencap_method_) {
    case MaaWin32ScreencapMethod_GDI:
        screencap_ = std::make_shared<GdiScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_FramePool:
        screencap_ = std::make_shared<FramePoolScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_DXGI_DesktopDup:
        screencap_ = std::make_shared<DesktopDupScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_DXGI_DesktopDup_Window:
        screencap_ = std::make_shared<DesktopDupWindowScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_PrintWindow:
        screencap_ = std::make_shared<PrintWindowScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_ScreenDC:
        screencap_ = std::make_shared<ScreenDCScreencap>(hwnd_);
        break;

    default:
        LogError << "Unknown screencap method: " << static_cast<int>(screencap_method_);
        break;
    }

    auto make_input = [&](MaaWin32InputMethod method) -> std::shared_ptr<InputBase> {
        switch (method) {
        case MaaWin32InputMethod_Seize:
            return std::make_shared<SeizeInput>(hwnd_, true);
        case MaaWin32InputMethod_SendMessage:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Mode::SendMessage, false, false);
        case MaaWin32InputMethod_PostMessage:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Mode::PostMessage, false, false);
        case MaaWin32InputMethod_LegacyEvent:
            return std::make_shared<LegacyEventInput>(hwnd_, true);
        case MaaWin32InputMethod_PostThreadMessage:
            return std::make_shared<PostThreadMessageInput>(hwnd_);
        case MaaWin32InputMethod_SendMessageWithCursorPos:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Mode::SendMessage, true, true);
        case MaaWin32InputMethod_PostMessageWithCursorPos:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Mode::PostMessage, true, true);
        case MaaWin32InputMethod_Gamepad:
            return std::make_shared<GamepadInput>();
        default:
            LogError << "Unknown input method: " << static_cast<int>(method);
            return nullptr;
        }
    };

    if (mouse_method_ == keyboard_method_) {
        mouse_ = make_input(mouse_method_);
        keyboard_ = mouse_;
    }
    else {
        mouse_ = make_input(mouse_method_);
        keyboard_ = make_input(keyboard_method_);
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

MaaControllerFeature Win32ControlUnitMgr::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_None;
    if (mouse_) {
        feat |= mouse_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    }
    if (keyboard_) {
        feat |= keyboard_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
    }
    return feat;
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
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->click(x, y);
}

bool Win32ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->swipe(x1, y1, x2, y2, duration);
}

bool Win32ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->touch_down(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->touch_move(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_up(int contact)
{
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->touch_up(contact);
}

bool Win32ControlUnitMgr::click_key(int key)
{
    if (!keyboard_) {
        LogError << "keyboard_ is null";
        return false;
    }

    return keyboard_->click_key(key);
}

bool Win32ControlUnitMgr::input_text(const std::string& text)
{
    if (!keyboard_) {
        LogError << "keyboard_ is null";
        return false;
    }

    return keyboard_->input_text(text);
}

bool Win32ControlUnitMgr::key_down(int key)
{
    if (!keyboard_) {
        LogError << "keyboard_ is null";
        return false;
    }

    return keyboard_->key_down(key);
}

bool Win32ControlUnitMgr::key_up(int key)
{
    if (!keyboard_) {
        LogError << "keyboard_ is null";
        return false;
    }

    return keyboard_->key_up(key);
}

bool Win32ControlUnitMgr::scroll(int dx, int dy)
{
    if (!mouse_) {
        LogError << "mouse_ is null";
        return false;
    }

    return mouse_->scroll(dx, dy);
}

MAA_CTRL_UNIT_NS_END
