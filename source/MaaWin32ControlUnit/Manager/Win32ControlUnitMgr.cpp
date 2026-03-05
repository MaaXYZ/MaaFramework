#include "Win32ControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"

#include "Input/LegacyEventInput.h"
#include "Input/MessageInput.h"
#include "Input/PostThreadMessageInput.h"
#include "Input/SeizeInput.h"
#include "Screencap/DesktopDupScreencap.h"
#include "Screencap/DesktopDupWindowScreencap.h"
#include "Screencap/FramePoolScreencap.h"
#include "Screencap/FramePoolWithPseudoMinimizeScreencap.h"
#include "Screencap/GdiScreencap.h"
#include "Screencap/HwndUtils.hpp"
#include "Screencap/PrintWindowScreencap.h"
#include "Screencap/PrintWindowWithPseudoMinimizeScreencap.h"
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

std::shared_ptr<ScreencapBase> Win32ControlUnitMgr::make_screencap(MaaWin32ScreencapMethod method) const
{
    switch (method) {
    case MaaWin32ScreencapMethod_GDI:
        return std::make_shared<GdiScreencap>(hwnd_);
    case MaaWin32ScreencapMethod_FramePool:
        return std::make_shared<FramePoolWithPseudoMinimizeScreencap>(hwnd_);
    case MaaWin32ScreencapMethod_DXGI_DesktopDup:
        return std::make_shared<DesktopDupScreencap>(hwnd_);
    case MaaWin32ScreencapMethod_DXGI_DesktopDup_Window:
        return std::make_shared<DesktopDupWindowScreencap>(hwnd_);
    case MaaWin32ScreencapMethod_PrintWindow:
        return std::make_shared<PrintWindowWithPseudoMinimizeScreencap>(hwnd_);
    case MaaWin32ScreencapMethod_ScreenDC:
        return std::make_shared<ScreenDCScreencap>(hwnd_);
    default:
        return nullptr;
    }
}

std::shared_ptr<ScreencapBase> Win32ControlUnitMgr::try_screencap_methods(
    const std::vector<MaaWin32ScreencapMethod>& methods) const
{
    for (auto method : methods) {
        auto unit = make_screencap(method);
        if (!unit) {
            LogWarn << "skip unsupported screencap method in auto mode" << VAR(method);
            continue;
        }

        if (unit->screencap()) {
            LogInfo << "auto screencap selected method" << VAR(method);
            return unit;
        }

        LogWarn << "auto screencap failed, try next" << VAR(method);
    }

    return nullptr;
}

bool Win32ControlUnitMgr::connect()
{
    connected_ = false;

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

        // FramePool 和 PrintWindow 内置伪最小化支持，允许最小化窗口
        bool supports_minimized = screencap_method_ == MaaWin32ScreencapMethod_FramePool
            || screencap_method_ == MaaWin32ScreencapMethod_PrintWindow
            || screencap_method_ == MaaWin32ScreencapMethod_AutoBackground;
        if (!supports_minimized && IsIconic(hwnd_)) {
            LogError << "hwnd_ is minimized";
            return false;
        }
    }
    else {
        LogWarn << "hwnd_ is nullptr";
    }

    switch (screencap_method_) {
    case MaaWin32ScreencapMethod_GDI:
    case MaaWin32ScreencapMethod_FramePool:
    case MaaWin32ScreencapMethod_DXGI_DesktopDup:
    case MaaWin32ScreencapMethod_DXGI_DesktopDup_Window:
    case MaaWin32ScreencapMethod_PrintWindow:
    case MaaWin32ScreencapMethod_ScreenDC:
        screencap_ = make_screencap(screencap_method_);
        break;
    case MaaWin32ScreencapMethod_AutoForeground:
        screencap_ = try_screencap_methods(
            { MaaWin32ScreencapMethod_DXGI_DesktopDup_Window, MaaWin32ScreencapMethod_GDI, MaaWin32ScreencapMethod_ScreenDC });
        break;
    case MaaWin32ScreencapMethod_AutoBackground:
        screencap_ = try_screencap_methods({ MaaWin32ScreencapMethod_FramePool, MaaWin32ScreencapMethod_PrintWindow });
        break;

    default:
        LogError << "Unknown screencap method: " << static_cast<int>(screencap_method_);
        break;
    }
    if (!screencap_) {
        LogError << "Failed to initialize screencap method:" << static_cast<int>(screencap_method_);
        return false;
    }

    auto make_input = [&](MaaWin32InputMethod method) -> std::shared_ptr<InputBase> {
        switch (method) {
        case MaaWin32InputMethod_Seize:
            return std::make_shared<SeizeInput>(hwnd_, false);
        case MaaWin32InputMethod_SendMessage:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Config { .mode = MessageInput::Mode::SendMessage });
        case MaaWin32InputMethod_PostMessage:
            return std::make_shared<MessageInput>(hwnd_, MessageInput::Config { .mode = MessageInput::Mode::PostMessage });
        case MaaWin32InputMethod_LegacyEvent:
            return std::make_shared<LegacyEventInput>(hwnd_, true);
        case MaaWin32InputMethod_PostThreadMessage:
            return std::make_shared<PostThreadMessageInput>(hwnd_);
        case MaaWin32InputMethod_SendMessageWithCursorPos:
            return std::make_shared<MessageInput>(
                hwnd_,
                MessageInput::Config { .mode = MessageInput::Mode::SendMessage, .with_cursor_pos = true, .block_input = false });
        case MaaWin32InputMethod_PostMessageWithCursorPos:
            return std::make_shared<MessageInput>(
                hwnd_,
                MessageInput::Config { .mode = MessageInput::Mode::PostMessage, .with_cursor_pos = true, .block_input = false });
        case MaaWin32InputMethod_SendMessageWithWindowPos:
            return std::make_shared<MessageInput>(
                hwnd_,
                MessageInput::Config { .mode = MessageInput::Mode::SendMessage, .with_window_pos = true, .block_input = false });
        case MaaWin32InputMethod_PostMessageWithWindowPos:
            return std::make_shared<MessageInput>(
                hwnd_,
                MessageInput::Config { .mode = MessageInput::Mode::PostMessage, .with_window_pos = true, .block_input = false });
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

    connected_ = true;
    return true;
}

bool Win32ControlUnitMgr::connected() const
{
    // 除了检查连接标志，还需要检查窗口是否仍然有效
    if (!connected_) {
        return false;
    }

    if (hwnd_ && !IsWindow(hwnd_)) {
        return false;
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

bool Win32ControlUnitMgr::inactive()
{
    LogFunc;

    if (screencap_) {
        screencap_->inactive();
    }
    if (mouse_) {
        mouse_->inactive();
    }
    if (keyboard_ && keyboard_ != mouse_) {
        keyboard_->inactive();
    }

    return true;
}

json::object Win32ControlUnitMgr::get_info() const
{
    json::object info;
    info["type"] = "win32";
    info["hwnd"] = reinterpret_cast<uint64_t>(hwnd_);
    info["screencap_method"] = static_cast<int64_t>(screencap_method_);
    info["mouse_method"] = static_cast<int64_t>(mouse_method_);
    info["keyboard_method"] = static_cast<int64_t>(keyboard_method_);
    return info;
}

MAA_CTRL_UNIT_NS_END
