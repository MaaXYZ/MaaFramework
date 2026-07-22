#include "InterceptionInput.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <thread>

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

constexpr DWORD kGenericRead = 0x80000000;
constexpr DWORD kOpenExisting = 3;

constexpr int kMaxDeviceCount = 20;
constexpr int kKeyboardDeviceCount = 10;
constexpr int kPreferredKeyboardDevice = 1;
constexpr int kMouseDeviceStart = 10;
constexpr int kMouseDeviceEnd = 19;

constexpr uint16_t kMouseMoveRelative = 0x0000;
constexpr uint16_t kMouseMoveAbsolute = 0x0001;
constexpr uint16_t kKeyUp = 0x0001;
constexpr uint16_t kKeyE0 = 0x0002;
constexpr uint16_t kKeyE1 = 0x0004;

constexpr uint16_t kMouseLeftButtonDown = 0x0001;
constexpr uint16_t kMouseLeftButtonUp = 0x0002;
constexpr uint16_t kMouseRightButtonDown = 0x0004;
constexpr uint16_t kMouseRightButtonUp = 0x0008;
constexpr uint16_t kMouseMiddleButtonDown = 0x0010;
constexpr uint16_t kMouseMiddleButtonUp = 0x0020;
constexpr uint16_t kMouseButton4Down = 0x0040;
constexpr uint16_t kMouseButton4Up = 0x0080;
constexpr uint16_t kMouseButton5Down = 0x0100;
constexpr uint16_t kMouseButton5Up = 0x0200;
constexpr uint16_t kMouseWheel = 0x0400;
constexpr uint16_t kMouseHWheel = 0x0800;

constexpr DWORD kFileDeviceUnknown = 0x00000022;
constexpr DWORD kMethodBuffered = 0;
constexpr DWORD kFileAnyAccess = 0;

constexpr DWORD ctl_code(DWORD device_type, DWORD function_code, DWORD method, DWORD access)
{
    return (device_type << 16) | (access << 14) | (function_code << 2) | method;
}

constexpr DWORD kIoctlSetEvent = ctl_code(kFileDeviceUnknown, 0x810, kMethodBuffered, kFileAnyAccess);
constexpr DWORD kIoctlWrite = ctl_code(kFileDeviceUnknown, 0x820, kMethodBuffered, kFileAnyAccess);
constexpr DWORD kIoctlGetHardwareId = ctl_code(kFileDeviceUnknown, 0x880, kMethodBuffered, kFileAnyAccess);

bool is_valid_mouse_index(int index)
{
    return index >= kMouseDeviceStart && index <= kMouseDeviceEnd;
}

bool is_valid_keyboard_index(int index)
{
    return index >= 0 && index < kKeyboardDeviceCount;
}

std::wstring device_path(int index)
{
    return std::format(LR"(\\.\interception{:02d})", index);
}

bool open_device_handle(int index, HANDLE& device_handle, HANDLE& event_handle)
{
    device_handle = CreateFileW(device_path(index).c_str(), kGenericRead, 0, nullptr, kOpenExisting, 0, nullptr);
    if (device_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    event_handle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!event_handle) {
        CloseHandle(device_handle);
        device_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    ULONG_PTR event_buffer[2] = { reinterpret_cast<ULONG_PTR>(event_handle), 0 };
    DWORD bytes_returned = 0;
    const BOOL ok = DeviceIoControl(
        device_handle,
        kIoctlSetEvent,
        event_buffer,
        static_cast<DWORD>(sizeof(event_buffer)),
        nullptr,
        0,
        &bytes_returned,
        nullptr);

    if (!ok) {
        CloseHandle(event_handle);
        CloseHandle(device_handle);
        event_handle = nullptr;
        device_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

void close_device_handle(HANDLE& device_handle, HANDLE& event_handle)
{
    if (device_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(device_handle);
        device_handle = INVALID_HANDLE_VALUE;
    }
    if (event_handle) {
        CloseHandle(event_handle);
        event_handle = nullptr;
    }
}

bool can_open_device(int index)
{
    HANDLE device_handle = INVALID_HANDLE_VALUE;
    HANDLE event_handle = nullptr;
    if (!open_device_handle(index, device_handle, event_handle)) {
        return false;
    }

    close_device_handle(device_handle, event_handle);
    return true;
}

bool query_hardware_id(int index, std::wstring& hardware_id)
{
    HANDLE device_handle = INVALID_HANDLE_VALUE;
    HANDLE event_handle = nullptr;
    if (!open_device_handle(index, device_handle, event_handle)) {
        return false;
    }

    std::array<wchar_t, 256> buffer { };
    DWORD bytes_returned = 0;
    const BOOL ok = DeviceIoControl(
        device_handle,
        kIoctlGetHardwareId,
        nullptr,
        0,
        buffer.data(),
        static_cast<DWORD>(sizeof(buffer)),
        &bytes_returned,
        nullptr);

    close_device_handle(device_handle, event_handle);

    if (!ok || bytes_returned == 0) {
        return false;
    }

    size_t char_count = std::min<size_t>(buffer.size() - 1, bytes_returned / sizeof(wchar_t));
    hardware_id.assign(buffer.data(), char_count);
    return !hardware_id.empty();
}

std::optional<int> find_mouse_device_index()
{
    for (int index = kMouseDeviceStart; index <= kMouseDeviceEnd; ++index) {
        std::wstring hardware_id;
        if (query_hardware_id(index, hardware_id)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<int> find_keyboard_device_index()
{
    if (can_open_device(kPreferredKeyboardDevice)) {
        return kPreferredKeyboardDevice;
    }

    for (int index = 0; index < kKeyboardDeviceCount; ++index) {
        if (index == kPreferredKeyboardDevice) {
            continue;
        }
        if (can_open_device(index)) {
            return index;
        }
    }
    return std::nullopt;
}

bool contact_to_interception_button(int contact, bool button_down, uint16_t& button_flag)
{
    const int mapped_contact = GetMappedContact(contact);
    switch (mapped_contact) {
    case 0:
        button_flag = button_down ? kMouseLeftButtonDown : kMouseLeftButtonUp;
        return true;
    case 1:
        button_flag = button_down ? kMouseRightButtonDown : kMouseRightButtonUp;
        return true;
    case 2:
        button_flag = button_down ? kMouseMiddleButtonDown : kMouseMiddleButtonUp;
        return true;
    case 3:
        button_flag = button_down ? kMouseButton4Down : kMouseButton4Up;
        return true;
    case 4:
        button_flag = button_down ? kMouseButton5Down : kMouseButton5Up;
        return true;
    default:
        return false;
    }
}

} // namespace

InterceptionInput::InterceptionInput(HWND hwnd)
    : hwnd_(hwnd)
{
}

InterceptionInput::~InterceptionInput()
{
    destroy_mouse_device();
    destroy_keyboard_device();
}

MaaControllerFeature InterceptionInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool InterceptionInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool InterceptionInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool InterceptionInput::touch_down(int contact, int x, int y, int pressure)
{
    if (!ensure_mouse_ready()) {
        return false;
    }

    if (!move_to_client_point(x, y)) {
        return false;
    }

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(mouse_device_index_) << VAR_VOIDP(hwnd_);

    if (!send_button(contact, true)) {
        return false;
    }

    last_pos_ = { x, y };
    last_pos_set_ = true;
    return true;
}

bool InterceptionInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = pressure;

    if (!ensure_mouse_ready()) {
        return false;
    }

    if (!move_to_client_point(x, y)) {
        return false;
    }

    last_pos_ = { x, y };
    last_pos_set_ = true;
    return true;
}

bool InterceptionInput::touch_up(int contact)
{
    if (!ensure_mouse_ready()) {
        return false;
    }

    LogInfo << VAR(contact) << VAR(mouse_device_index_) << VAR_VOIDP(hwnd_);
    return send_button(contact, false);
}

bool InterceptionInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool InterceptionInput::input_text(const std::string& text)
{
    auto u16_text = to_u16(text);
    LogInfo << VAR(text) << VAR(u16_text) << VAR(mouse_device_index_) << VAR(keyboard_device_index_) << VAR_VOIDP(hwnd_);

    for (const auto ch : u16_text) {
        INPUT input = { };
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        input.ki.wScan = ch;

        SendInput(1, &input, sizeof(INPUT));

        input.ki.dwFlags |= KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    return true;
}

bool InterceptionInput::key_down(int key)
{
    if (!ensure_keyboard_ready()) {
        return false;
    }

    ensure_foreground();

    LogInfo << VAR(key) << VAR(keyboard_device_index_) << VAR_VOIDP(hwnd_);
    return send_key(key, false);
}

bool InterceptionInput::key_up(int key)
{
    if (!ensure_keyboard_ready()) {
        return false;
    }

    ensure_foreground();

    LogInfo << VAR(key) << VAR(keyboard_device_index_) << VAR_VOIDP(hwnd_);
    return send_key(key, true);
}

bool InterceptionInput::scroll(int dx, int dy)
{
    if (!ensure_mouse_ready()) {
        return false;
    }

    LogInfo << VAR(dx) << VAR(dy) << VAR(mouse_device_index_) << VAR_VOIDP(hwnd_);

    auto [target_x, target_y] = get_target_pos();
    if (!move_to_client_point(target_x, target_y)) {
        return false;
    }

    bool ok = true;
    if (dy != 0) {
        ok &= send_scroll_axis(dy, false);
    }
    if (dx != 0) {
        ok &= send_scroll_axis(dx, true);
    }
    return ok;
}

void InterceptionInput::inactive()
{
    destroy_mouse_device();
    destroy_keyboard_device();
}

bool InterceptionInput::ensure_mouse_ready()
{
    if (mouse_device_handle_ != INVALID_HANDLE_VALUE && is_valid_mouse_index(mouse_device_index_)) {
        return true;
    }
    return initialize_mouse_device();
}

bool InterceptionInput::ensure_keyboard_ready()
{
    if (keyboard_device_handle_ != INVALID_HANDLE_VALUE && is_valid_keyboard_index(keyboard_device_index_)) {
        return true;
    }
    return initialize_keyboard_device();
}

bool InterceptionInput::initialize_mouse_device()
{
    destroy_mouse_device();

    auto mouse_index = find_mouse_device_index();
    if (!mouse_index) {
        LogError << "Interception driver not found or no mouse device available";
        return false;
    }

    if (!open_device_handle(*mouse_index, mouse_device_handle_, mouse_event_handle_)) {
        LogError << "Failed to open Interception mouse device" << VAR(*mouse_index) << VAR(GetLastError());
        destroy_mouse_device();
        return false;
    }

    mouse_device_index_ = *mouse_index;
    LogInfo << "Interception mouse device initialized" << VAR(mouse_device_index_);
    return true;
}

bool InterceptionInput::initialize_keyboard_device()
{
    destroy_keyboard_device();

    auto keyboard_index = find_keyboard_device_index();
    if (!keyboard_index) {
        LogError << "Interception driver not found or no keyboard device available";
        return false;
    }

    if (!open_device_handle(*keyboard_index, keyboard_device_handle_, keyboard_event_handle_)) {
        LogError << "Failed to open Interception keyboard device" << VAR(*keyboard_index) << VAR(GetLastError());
        destroy_keyboard_device();
        return false;
    }

    keyboard_device_index_ = *keyboard_index;
    LogInfo << "Interception keyboard device initialized" << VAR(keyboard_device_index_);
    return true;
}

void InterceptionInput::destroy_mouse_device()
{
    close_device_handle(mouse_device_handle_, mouse_event_handle_);
    mouse_device_index_ = -1;
}

void InterceptionInput::destroy_keyboard_device()
{
    close_device_handle(keyboard_device_handle_, keyboard_event_handle_);
    keyboard_device_index_ = -1;
}

bool InterceptionInput::ensure_foreground()
{
    if (!hwnd_) {
        return true;
    }

    ensure_foreground_and_topmost(hwnd_);
    if (hwnd_ != GetForegroundWindow()) {
        LogWarn << "Failed to ensure foreground window before Interception keyboard input" << VAR_VOIDP(hwnd_);
        return false;
    }
    return true;
}

bool InterceptionInput::send_mouse_stroke(const MouseStroke& stroke)
{
    if (mouse_device_handle_ == INVALID_HANDLE_VALUE) {
        LogError << "Interception device handle is invalid";
        return false;
    }

    DWORD bytes_returned = 0;
    const BOOL ok = DeviceIoControl(
        mouse_device_handle_,
        kIoctlWrite,
        const_cast<MouseStroke*>(&stroke),
        static_cast<DWORD>(sizeof(stroke)),
        nullptr,
        0,
        &bytes_returned,
        nullptr);

    if (!ok) {
        LogError << "Interception DeviceIoControl(IOCTL_WRITE) failed" << VAR(GetLastError()) << VAR(mouse_device_index_);
        return false;
    }

    return true;
}

bool InterceptionInput::send_keyboard_stroke(const KeyboardStroke& stroke)
{
    if (keyboard_device_handle_ == INVALID_HANDLE_VALUE) {
        LogError << "Interception keyboard device handle is invalid";
        return false;
    }

    DWORD bytes_returned = 0;
    const BOOL ok = DeviceIoControl(
        keyboard_device_handle_,
        kIoctlWrite,
        const_cast<KeyboardStroke*>(&stroke),
        static_cast<DWORD>(sizeof(stroke)),
        nullptr,
        0,
        &bytes_returned,
        nullptr);

    if (!ok) {
        LogError << "Interception keyboard DeviceIoControl(IOCTL_WRITE) failed" << VAR(GetLastError()) << VAR(keyboard_device_index_);
        return false;
    }

    return true;
}

bool InterceptionInput::move_to_client_point(int x, int y)
{
    auto screen_point = client_to_screen_point(x, y);
    if (!screen_point) {
        return false;
    }

    auto interception_point = to_interception_coordinate(*screen_point);
    if (!interception_point) {
        return false;
    }

    auto [ix, iy] = *interception_point;
    LogDebug << "Interception move_to_client_point" << VAR(x) << VAR(y) << VAR(screen_point->x) << VAR(screen_point->y) << VAR(ix)
             << VAR(iy) << VAR(mouse_device_index_) << VAR_VOIDP(hwnd_);
    MouseStroke stroke;
    stroke.flags = kMouseMoveAbsolute;
    stroke.x = ix;
    stroke.y = iy;
    return send_mouse_stroke(stroke);
}

bool InterceptionInput::send_button(int contact, bool button_down)
{
    uint16_t button_flag = 0;
    if (!contact_to_interception_button(contact, button_down, button_flag)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    const int mapped_contact = GetMappedContact(contact);
    LogDebug << "Interception send_button" << VAR(contact) << VAR(mapped_contact) << VAR(button_down) << VAR(button_flag)
             << VAR(mouse_device_index_) << VAR_VOIDP(hwnd_);

    MouseStroke stroke;
    stroke.flags = kMouseMoveRelative;
    stroke.button_flags = button_flag;
    return send_mouse_stroke(stroke);
}

bool InterceptionInput::send_scroll_axis(int delta, bool horizontal)
{
    MouseStroke stroke;
    stroke.flags = kMouseMoveRelative;
    stroke.button_flags = horizontal ? kMouseHWheel : kMouseWheel;
    stroke.button_data = static_cast<uint16_t>(static_cast<int16_t>(delta));
    return send_mouse_stroke(stroke);
}

bool InterceptionInput::send_key(int key, bool key_up)
{
    const UINT scan_code_ex = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC_EX);
    if (scan_code_ex == 0) {
        LogError << "MapVirtualKeyW returned invalid scan code" << VAR(key);
        return false;
    }

    KeyboardStroke stroke;
    stroke.code = static_cast<uint16_t>(scan_code_ex & 0xFF);
    stroke.state = key_up ? kKeyUp : 0;

    const UINT prefix = scan_code_ex & 0xFF00;
    if (prefix == 0xE000) {
        stroke.state |= kKeyE0;
    }
    else if (prefix == 0xE100) {
        stroke.state |= kKeyE1;
    }

    LogDebug << "Interception send_key" << VAR(key) << VAR(scan_code_ex) << VAR(stroke.code) << VAR(stroke.state) << VAR(key_up)
             << VAR(keyboard_device_index_) << VAR_VOIDP(hwnd_);

    return send_keyboard_stroke(stroke);
}

std::pair<int, int> InterceptionInput::get_target_pos() const
{
    if (last_pos_set_) {
        return last_pos_;
    }

    RECT rect { };
    if (hwnd_ && GetClientRect(hwnd_, &rect)) {
        return { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    }
    return { 0, 0 };
}

std::optional<POINT> InterceptionInput::client_to_screen_point(int x, int y) const
{
    POINT point { x, y };
    if (!hwnd_) {
        return point;
    }

    if (!ClientToScreen(hwnd_, &point)) {
        LogError << "ClientToScreen failed" << VAR_VOIDP(hwnd_) << VAR(x) << VAR(y) << VAR(GetLastError());
        return std::nullopt;
    }
    return point;
}

std::optional<std::pair<int32_t, int32_t>> InterceptionInput::to_interception_coordinate(const POINT& point) const
{
    const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = GetSystemMetrics(SM_CYSCREEN);
    if (screen_width <= 0 || screen_height <= 0) {
        LogError << "GetSystemMetrics returned invalid screen size" << VAR(screen_width) << VAR(screen_height);
        return std::nullopt;
    }

    const int point_x = static_cast<int>(point.x);
    const int point_y = static_cast<int>(point.y);
    const int clamped_x = std::clamp(point_x, 0, screen_width - 1);
    const int clamped_y = std::clamp(point_y, 0, screen_height - 1);
    const int32_t x = static_cast<int32_t>((static_cast<int64_t>(clamped_x) * 0xFFFF) / screen_width);
    const int32_t y = static_cast<int32_t>((static_cast<int64_t>(clamped_y) * 0xFFFF) / screen_height);
    return std::make_pair(x, y);
}

MAA_CTRL_UNIT_NS_END
