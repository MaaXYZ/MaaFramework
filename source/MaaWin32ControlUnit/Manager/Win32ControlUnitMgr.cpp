#include "Win32ControlUnitMgr.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "Win32TargetSelector.h"

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Time.hpp"
#include "MaaUtils/Uuid.h"

#include <tlhelp32.h>

#include "Input/AnchoredTouchInput.h"
#include "Input/BackgroundManagedKeyInput.h"
#include "Input/InterceptionInput.h"
#include "Input/LegacyEventInput.h"
#include "Input/MessageInput.h"
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
    : initial_hwnd_(hWnd)
    , screencap_method_(screencap_method)
    , mouse_method_(mouse_method)
    , keyboard_method_(keyboard_method)
{
    target_.hwnd = hWnd;
    target_.background_keyboard = std::make_shared<BackgroundManagedKeyInput>(hWnd);
}

bool Win32ControlUnitMgr::Target::available() const
{
    if (!ready || lost || !hwnd) {
        return ready && !lost;
    }

    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    lost = !tid || tid != thread_id || pid != process_id;
    return !lost;
}

bool Win32ControlUnitMgr::set_deferred_binding()
{
    std::scoped_lock lock(connection_mutex_, mutex_);
    if (connected_ || target_.hwnd) {
        return false;
    }
    independent_connection_ = true;
    target_ = { };
    if (session_uuid_.empty()) {
        session_uuid_ = make_uuid();
    }
    return true;
}

bool Win32ControlUnitMgr::connect()
{
    std::scoped_lock lock(connection_mutex_, mutex_);
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

    if (independent_connection_) {
        connected_ = true;
        return true;
    }

    target_.ready = false;
    target_.lost = false;
    target_.mouse.reset();
    target_.keyboard.reset();
    target_.screencap.reset();
    connected_ = prepare_target(target_, false);
    return connected_;
}

namespace
{
bool supports_desktop_input(MaaWin32InputMethod method)
{
    return method == MaaWin32InputMethod_None || method == MaaWin32InputMethod_Seize || method == MaaWin32InputMethod_LegacyEvent
           || method == MaaWin32InputMethod_Interception;
}
}

bool Win32ControlUnitMgr::prepare_target(Target& target, bool explicit_bind, const std::function<bool()>& cancelled) const
{
    auto capture_methods = screencap_method_;
    if (target.hwnd) {
        target.thread_id = GetWindowThreadProcessId(target.hwnd, &target.process_id);
        if (explicit_bind && capture_methods) {
            capture_methods &= ~MaaWin32ScreencapMethod_DXGI_DesktopDup;
            if (!capture_methods) {
                LogError << "Window Bind requires a window-relative screencap method";
                return false;
            }
        }
        if (!target.thread_id) {
            LogError << "Invalid target window" << VAR_VOIDP(target.hwnd);
            return false;
        }
    }
    else if (explicit_bind) {
        if (!supports_desktop_input(mouse_method_) || !supports_desktop_input(keyboard_method_)
            || (capture_methods && !(capture_methods & MaaWin32ScreencapMethod_DXGI_DesktopDup))) {
            LogError << "Requested methods cannot operate on the primary desktop" << VAR(capture_methods) << VAR(mouse_method_)
                     << VAR(keyboard_method_);
            return false;
        }
        capture_methods &= MaaWin32ScreencapMethod_DXGI_DesktopDup;
    }

    if (!init_screencap(target, capture_methods, cancelled)) {
        return false;
    }
    target.mouse = make_input(target.hwnd, mouse_method_);
    target.keyboard = mouse_method_ == keyboard_method_ ? target.mouse : make_input(target.hwnd, keyboard_method_);
    if (explicit_bind && ((mouse_method_ && !target.mouse) || (keyboard_method_ && !target.keyboard))) {
        return false;
    }
    target.ready = true;
    return target.available();
}

bool Win32ControlUnitMgr::bind_target(
    const json::object& target,
    const std::function<bool()>& cancelled,
    const std::function<void()>& before_switch)
{
    std::lock_guard connection_lock(connection_mutex_);
    {
        std::lock_guard lock(mutex_);
        if (!connected_) {
            LogError << "Bind requires a connected controller";
            return false;
        }
    }
    if (cancelled && cancelled()) {
        return false;
    }
    const auto hwnd = resolve_bind_target(target, cancelled);
    if (!hwnd) {
        return false;
    }
    {
        std::lock_guard lock(mutex_);
        if (independent_connection_ && target_.hwnd == *hwnd && target_.available()) {
            return !cancelled || !cancelled();
        }
    }

    Target candidate;
    candidate.hwnd = *hwnd;
    candidate.background_keyboard = std::make_shared<BackgroundManagedKeyInput>(*hwnd);
    if (!prepare_target(candidate, true, cancelled) || (cancelled && cancelled())) {
        return false;
    }

    // The callback can call key_up/touch_up on this control unit.
    if (before_switch) {
        before_switch();
    }
    std::lock_guard lock(mutex_);
    if (!candidate.available() || (cancelled && cancelled())) {
        return false;
    }
    if (target_.background_keyboard) {
        if (!target_.background_keyboard->rebind(candidate.hwnd, managed_keys_)) {
            return false;
        }
        candidate.background_keyboard = std::move(target_.background_keyboard);
    }
    else if (!managed_keys_.empty()) {
        const std::vector<int> keys(managed_keys_.begin(), managed_keys_.end());
        if (!candidate.background_keyboard->set_managed_keys(keys)) {
            return false;
        }
    }
    target_.inactive();
    target_ = std::move(candidate);
    independent_connection_ = true;
    return true;
}

std::unordered_map<Win32ControlUnitMgr::ScreencapMethod, std::shared_ptr<ScreencapBase>>
    Win32ControlUnitMgr::build_screencap_units(HWND hwnd, MaaWin32ScreencapMethod methods) const
{
    std::unordered_map<ScreencapMethod, std::shared_ptr<ScreencapBase>> units;

    const auto add = [&](MaaWin32ScreencapMethod flag, ScreencapMethod method, auto factory) {
        if (methods & flag) {
            units.emplace(method, factory());
        }
    };

    add(MaaWin32ScreencapMethod_GDI, ScreencapMethod::GDI, [hwnd] { return std::make_shared<GdiScreencap>(hwnd); });
    add(MaaWin32ScreencapMethod_FramePool, ScreencapMethod::FramePool, [hwnd] {
        return std::make_shared<FramePoolWithPseudoMinimizeScreencap>(hwnd);
    });
    add(MaaWin32ScreencapMethod_DXGI_DesktopDup, ScreencapMethod::DXGI_DesktopDup, [hwnd] {
        return std::make_shared<DesktopDupScreencap>(hwnd);
    });
    add(MaaWin32ScreencapMethod_DXGI_DesktopDup_Window, ScreencapMethod::DXGI_DesktopDup_Window, [hwnd] {
        return std::make_shared<DesktopDupWindowScreencap>(hwnd);
    });
    add(MaaWin32ScreencapMethod_PrintWindow, ScreencapMethod::PrintWindow, [hwnd] {
        return std::make_shared<PrintWindowWithPseudoMinimizeScreencap>(hwnd);
    });
    add(MaaWin32ScreencapMethod_ScreenDC, ScreencapMethod::ScreenDC, [hwnd] { return std::make_shared<ScreenDCScreencap>(hwnd); });

    return units;
}

bool Win32ControlUnitMgr::init_screencap(Target& target, MaaWin32ScreencapMethod methods, const std::function<bool()>& cancelled) const
{
    if (methods == MaaWin32ScreencapMethod_None) {
        LogWarn << "No screencap method selected";
        return true;
    }

    auto units = build_screencap_units(target.hwnd, methods);
    if (units.empty()) {
        LogError << "No available screencap method to test";
        return false;
    }

    target.screencap = speed_test(units, cancelled);
    if (!target.screencap) {
        LogError << "failed to select screencap method";
        return false;
    }

    return true;
}

std::shared_ptr<InputBase> Win32ControlUnitMgr::make_input(HWND hwnd, MaaWin32InputMethod method) const
{
    switch (method) {
    case MaaWin32InputMethod_None:
        return nullptr;
    case MaaWin32InputMethod_Seize:
        return std::make_shared<SeizeInput>(hwnd, false);
    case MaaWin32InputMethod_SendMessage:
        return std::make_shared<MessageInput>(hwnd, MessageInput::Config { .mode = MessageInput::Mode::SendMessage });
    case MaaWin32InputMethod_PostMessage:
        return std::make_shared<MessageInput>(hwnd, MessageInput::Config { .mode = MessageInput::Mode::PostMessage });
    case MaaWin32InputMethod_LegacyEvent:
        return std::make_shared<LegacyEventInput>(hwnd, true);
    case MaaWin32InputMethod_PostThreadMessage:
        LogError << "MaaWin32InputMethod_PostThreadMessage is deprecated and no longer implemented";
        return nullptr;
    case MaaWin32InputMethod_SendMessageWithCursorPos:
        return std::make_shared<MessageInput>(
            hwnd,
            MessageInput::Config { .mode = MessageInput::Mode::SendMessage, .with_cursor_pos = true, .block_input = true });
    case MaaWin32InputMethod_PostMessageWithCursorPos:
        return std::make_shared<MessageInput>(
            hwnd,
            MessageInput::Config { .mode = MessageInput::Mode::PostMessage, .with_cursor_pos = true, .block_input = true });
    case MaaWin32InputMethod_SendMessageWithWindowPos:
        return std::make_shared<MessageInput>(
            hwnd,
            MessageInput::Config {
                .mode = MessageInput::Mode::SendMessage,
                .with_window_pos = true,
                .track_hardware_mouse = false,
                .block_input = false,
            });
    case MaaWin32InputMethod_PostMessageWithWindowPos:
        return std::make_shared<MessageInput>(
            hwnd,
            MessageInput::Config {
                .mode = MessageInput::Mode::PostMessage,
                .with_window_pos = true,
                .track_hardware_mouse = false,
                .block_input = false,
            });
    case MaaWin32InputMethod_Interception:
        return std::make_shared<InterceptionInput>(hwnd);
    case MaaWin32InputMethod_AnchoredTouch:
        return std::make_shared<AnchoredTouchInput>(hwnd);
    default:
        LogError << "Unknown input method: " << static_cast<int>(method);
        return nullptr;
    }
}

std::shared_ptr<ScreencapBase> Win32ControlUnitMgr::speed_test(
    const std::unordered_map<ScreencapMethod, std::shared_ptr<ScreencapBase>>& units,
    const std::function<bool()>& cancelled) const
{
    LogFunc;

    ScreencapMethod fastest = ScreencapMethod::UnknownYet;
    auto cost = std::chrono::milliseconds::max();

    auto check = [&fastest, &cost](ScreencapMethod method, std::chrono::steady_clock::time_point start) {
        auto duration = duration_since(start);
        if (duration < cost) {
            fastest = method;
            cost = duration;
        }
        LogInfo << VAR(method) << VAR(duration);
    };

    for (auto& [method, unit] : units) {
        if (cancelled && cancelled()) {
            return nullptr;
        }
        LogInfo << "Warming up" << method;
        if (!unit->screencap()) {
            LogWarn << "failed to warm up" << method;
        }

        if (cancelled && cancelled()) {
            return nullptr;
        }
        LogInfo << "Testing" << method;
        auto now = std::chrono::steady_clock::now();
        if (!unit->screencap()) {
            LogWarn << "failed to test" << method;
            continue;
        }
        check(method, now);
    }

    if (fastest == ScreencapMethod::UnknownYet) {
        LogError << "cannot find any method to screencap!";
        return nullptr;
    }

    LogInfo << "The fastest method is" << fastest << VAR(cost);
    return units.at(fastest);
}

bool Win32ControlUnitMgr::connected() const
{
    std::lock_guard lock(mutex_);
    return connected_ && (independent_connection_ || target_.available());
}

bool Win32ControlUnitMgr::target_available() const
{
    std::lock_guard lock(mutex_);
    return target_.available();
}

bool Win32ControlUnitMgr::request_uuid(std::string& uuid)
{
    std::lock_guard lock(mutex_);
    if (!session_uuid_.empty()) {
        uuid = session_uuid_;
        return true;
    }
    std::stringstream ss;
    ss << initial_hwnd_;
    uuid = std::move(ss).str();

    return true;
}

MaaControllerFeature Win32ControlUnitMgr::get_features() const
{
    std::lock_guard lock(mutex_);
    if (!target_.available()) {
        return MaaControllerFeature_None;
    }
    MaaControllerFeature feat = MaaControllerFeature_None;
    if (target_.mouse) {
        feat |= target_.mouse->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    }
    if (target_.keyboard) {
        feat |= target_.keyboard->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
    }
    return feat;
}

bool Win32ControlUnitMgr::start_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (intent.empty()) {
        LogError << "intent is empty";
        return false;
    }

    std::wstring cmd = MAA_NS::to_u16(intent);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        LogInfo << "CreateProcessW succeeded, PID:" << pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    DWORD err = GetLastError();
    if (err != ERROR_ELEVATION_REQUIRED) {
        LogError << "CreateProcessW failed, error:" << err;
        return false;
    }
    LogInfo << "Elevation required. Falling back to ShellExecuteExW...";
    std::wstring file = cmd;
    std::wstring args;
    // Parse
    if (cmd[0] == L'\"') {
        auto pos = cmd.find(L'\"', 1);
        if (pos == std::wstring::npos) {
            LogError << "Mismatched quotes in command, cannot parse file and arguments";
            return false;
        }
        file = cmd.substr(1, pos - 1);
        if (pos + 1 < cmd.length()) {
            args = cmd.substr(pos + 1);
            args.erase(0, args.find_first_not_of(L" \t"));
        }
    }
    else if (auto pos = cmd.find(L' '); pos != std::wstring::npos) {
        file = cmd.substr(0, pos);
        args = cmd.substr(pos + 1);
        args.erase(0, args.find_first_not_of(L" \t"));
    }
    // execute
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = file.c_str();
    sei.lpParameters = args.empty() ? nullptr : args.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        LogError << "ShellExecuteExW failed, error:" << GetLastError();
        return false;
    }
    LogInfo << "ShellExecuteExW succeeded.";
    if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }
    return true;
}

bool Win32ControlUnitMgr::stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (intent.empty()) {
        std::lock_guard lock(mutex_);
        if (!target_.hwnd || !target_.available()) {
            LogError << "target_.hwnd is invalid and intent is empty";
            return false;
        }

        DWORD processId = 0;
        GetWindowThreadProcessId(target_.hwnd, &processId);
        if (processId == 0) {
            LogError << "GetWindowThreadProcessId failed, error:" << GetLastError();
            return false;
        }

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (!hProcess) {
            LogError << "OpenProcess failed, error:" << GetLastError();
            return false;
        }

        if (!TerminateProcess(hProcess, 0)) {
            LogError << "TerminateProcess failed, error:" << GetLastError();
            CloseHandle(hProcess);
            return false;
        }

        CloseHandle(hProcess);
        LogInfo << "Process" << processId << "terminated successfully by hwnd";
        return true;
    }

    std::wstring target_exe = MAA_NS::to_u16(intent);
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        LogError << "CreateToolhelp32Snapshot failed, error:" << GetLastError();
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    bool terminated_any = false;

    if (!Process32FirstW(hSnap, &pe)) {
        LogError << "Process32FirstW failed, error:" << GetLastError();
        CloseHandle(hSnap);
        return false;
    }

    do {
        if (target_exe != pe.szExeFile) {
            continue;
        }

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
        if (!hProcess) {
            LogError << "OpenProcess failed for PID" << pe.th32ProcessID << "error:" << GetLastError();
            continue;
        }

        if (TerminateProcess(hProcess, 0)) {
            LogInfo << "Process" << pe.th32ProcessID << "terminated successfully by intent matching";
            terminated_any = true;
        }
        else {
            LogError << "TerminateProcess failed for PID" << pe.th32ProcessID << "error:" << GetLastError();
        }

        CloseHandle(hProcess);

    } while (Process32NextW(hSnap, &pe));
    CloseHandle(hSnap);

    return terminated_any;
}

bool Win32ControlUnitMgr::screencap(cv::Mat& image)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.screencap) {
        LogError << "Target screencap is unavailable";
        return false;
    }

    auto opt = target_.screencap->screencap();
    if (!opt) {
        LogError << "failed to screencap";
        return false;
    }

    image = std::move(opt).value();

    return true;
}

bool Win32ControlUnitMgr::click(int x, int y)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->click(x, y);
}

bool Win32ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->swipe(x1, y1, x2, y2, duration);
}

bool Win32ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->touch_down(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->touch_move(contact, x, y, pressure);
}

bool Win32ControlUnitMgr::touch_up(int contact)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->touch_up(contact);
}

bool Win32ControlUnitMgr::relative_move(int dx, int dy)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    auto relative_move_input = std::dynamic_pointer_cast<RelativeMoveInput>(target_.mouse);
    if (!relative_move_input) {
        LogError << "relative_move is not supported by the current Win32 input method";
        return false;
    }

    return relative_move_input->relative_move(dx, dy);
}

bool Win32ControlUnitMgr::click_key(int key)
{
    std::lock_guard lock(mutex_);
    if (!target_.available()) {
        return false;
    }
    if (managed_keys_.contains(key)) {
        if (!target_.background_keyboard) {
            LogError << "target_.background_keyboard is null";
            return false;
        }
        const bool key_down_ok = target_.background_keyboard->key_down(key);
        const bool key_up_ok = target_.background_keyboard->key_up(key);
        if (key_down_ok && !key_up_ok) {
            LogError << "Managed key" << VAR(key) << "key_down succeeded but key_up failed; key may remain logically pressed";
        }
        return key_down_ok && key_up_ok;
    }

    if (!target_.keyboard) {
        LogError << "target_.keyboard is null";
        return false;
    }

    return target_.keyboard->click_key(key);
}

bool Win32ControlUnitMgr::input_text(const std::string& text)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.keyboard) {
        LogError << "Target keyboard is unavailable";
        return false;
    }

    return target_.keyboard->input_text(text);
}

bool Win32ControlUnitMgr::key_down(int key)
{
    std::lock_guard lock(mutex_);
    if (!target_.available()) {
        return false;
    }
    if (managed_keys_.contains(key)) {
        if (!target_.background_keyboard) {
            LogError << "target_.background_keyboard is null";
            return false;
        }
        return target_.background_keyboard->key_down(key);
    }

    if (!target_.keyboard) {
        LogError << "target_.keyboard is null";
        return false;
    }

    return target_.keyboard->key_down(key);
}

bool Win32ControlUnitMgr::key_up(int key)
{
    std::lock_guard lock(mutex_);
    if (!target_.available()) {
        return false;
    }
    if (managed_keys_.contains(key)) {
        if (!target_.background_keyboard) {
            LogError << "target_.background_keyboard is null";
            return false;
        }
        return target_.background_keyboard->key_up(key);
    }

    if (!target_.keyboard) {
        LogError << "target_.keyboard is null";
        return false;
    }

    return target_.keyboard->key_up(key);
}

bool Win32ControlUnitMgr::scroll(int dx, int dy)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    return target_.mouse->scroll(dx, dy);
}

bool Win32ControlUnitMgr::set_mouse_lock_follow(bool enabled)
{
    std::lock_guard lock(mutex_);
    if (!target_.available() || !target_.mouse) {
        LogError << "Target mouse is unavailable";
        return false;
    }

    auto message_input = std::dynamic_pointer_cast<MessageInput>(target_.mouse);
    if (!message_input) {
        LogError << "Mouse lock follow is only supported by MessageInput-based input methods";
        return false;
    }

    return message_input->set_mouse_lock_follow(enabled);
}

bool Win32ControlUnitMgr::set_background_managed_keys_option(const int32_t* keycodes, size_t count)
{
    LogFunc << VAR(count);
    std::lock_guard lock(mutex_);
    std::vector<int> keys;
    if (count) {
        if (!keycodes) {
            return false;
        }
        keys.assign(keycodes, keycodes + count);
    }
    if (std::ranges::any_of(keys, [](int key) { return key <= 0 || key > 0xFF; })) {
        return false;
    }
    // Deferred sessions remember the configuration without starting a global keyboard guard.
    if (target_.background_keyboard && !target_.background_keyboard->set_managed_keys(keys)) {
        LogError << "set_managed_keys failed";
        return false;
    }
    managed_keys_ = std::unordered_set<int>(keys.begin(), keys.end());
    return true;
}

void Win32ControlUnitMgr::Target::inactive()
{
    if (background_keyboard) {
        background_keyboard->inactive();
    }
    if (screencap) {
        screencap->inactive();
    }
    if (mouse) {
        mouse->inactive();
    }
    if (keyboard && keyboard != mouse) {
        keyboard->inactive();
    }
}

bool Win32ControlUnitMgr::inactive()
{
    std::lock_guard lock(mutex_);
    target_.inactive();
    return true;
}

json::object Win32ControlUnitMgr::get_info() const
{
    std::lock_guard lock(mutex_);
    json::object info;
    if (!target_.ready) {
        info["target_state"] = "Unbound";
    }
    else if (!target_.available()) {
        info["target_state"] = "Lost";
    }
    else {
        info["target_state"] = target_.hwnd ? "Window" : "Desktop";
    }
    info["type"] = "win32";
    info["hwnd"] = reinterpret_cast<uint64_t>(target_.hwnd);
    info["screencap_method"] = static_cast<int64_t>(screencap_method_);
    info["mouse_method"] = static_cast<int64_t>(mouse_method_);
    info["keyboard_method"] = static_cast<int64_t>(keyboard_method_);
    return info;
}

MAA_CTRL_UNIT_NS_END
