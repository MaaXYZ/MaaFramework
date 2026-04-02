#include "GlobalEventInput.h"

#include "InputUtils.h"
#include "MaaUtils/Logger.h"

#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <tuple>
#include <vector>
#include <unistd.h>

MAA_CTRL_UNIT_NS_BEGIN

MaaControllerFeature GlobalEventInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 touch_down/touch_up 替代 click/swipe
bool GlobalEventInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool GlobalEventInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool GlobalEventInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    update_window_info();

    // 记录当前鼠标位置，touch_up 后恢复
    CGEventRef cur = CGEventCreate(nullptr);
    if (cur != nullptr) {
        saved_cursor_pos_ = CGEventGetLocation(cur);
        CFRelease(cur);
    } else {
        LogWarn << "Failed to create CGEvent for current cursor position, saved_cursor_pos_ not updated";
    }

    MouseEventInfo info;
    if (!contact_to_mouse_down_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, touch may not work";
    }

    if (!post_mouse_event(info.event_type, CGPointMake(x + offset_x_, y + offset_y_), info.mouse_button)) {
        LogError << "Failed to post mouse down event";
        return false;
    }

    return true;
}

bool GlobalEventInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    MouseEventInfo info;
    if (!contact_to_mouse_move_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    if (!post_mouse_event(info.event_type, CGPointMake(x + offset_x_, y + offset_y_), info.mouse_button)) {
        LogError << "Failed to post mouse dragged event";
        return false;
    }

    return true;
}

bool GlobalEventInput::touch_up(int contact)
{
    MouseEventInfo info;
    if (!contact_to_mouse_up_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    // 获取当前鼠标位置作为释放位置
    CGPoint pos = {};
    CGEventRef cur = CGEventCreate(nullptr);
    if (cur != nullptr) {
        pos = CGEventGetLocation(cur);
        CFRelease(cur);
    }
    else {
        LogWarn << "Failed to create CGEvent for current cursor position";
    }

    bool result = post_mouse_event(info.event_type, pos, info.mouse_button);
    if (!result) {
        LogError << "Failed to post mouse up event";
    }

    // 无论 post_mouse_event 是否成功，均恢复鼠标到 touch_down 之前的位置
    if (!std::isnan(saved_cursor_pos_.x)) {
        CGWarpMouseCursorPosition(saved_cursor_pos_);
        saved_cursor_pos_ = { NAN, NAN };
    }

    return result;
}

// get_features() 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 key_down/key_up 替代 click_key
bool GlobalEventInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool GlobalEventInput::input_text(const std::string& text)
{
    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, input_text may not work";
    }

    // 将 UTF-8 转换为 UTF-16（UniChar），直接通过 CGEventKeyboardSetUnicodeString 注入，
    // 无需 keycode 映射，支持任意 Unicode 字符（含中文、emoji 等）
    std::vector<UniChar> chars;
    if (!text_to_unichars(text, chars)) {
        LogError << "Failed to convert text to UniChar: " << text;
        return false;
    }

    // key_down + key_up 各发一次，接收方通常只处理 key_down
    for (bool is_down : { true, false }) {
        CGEventRef event = CGEventCreateKeyboardEvent(nullptr, 0, is_down);
        if (!event) {
            LogError << "Failed to create keyboard event";
            return false;
        }
        CGEventKeyboardSetUnicodeString(event, chars.size(), chars.data());
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
        usleep(10000);
    }

    return true;
}

bool GlobalEventInput::key_down(int key)
{
    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, key_down may not work";
    }

    // 键盘按下
    if (!post_keyboard_event((CGKeyCode)key, true)) {
        LogError << "Failed to post keyboard down event";
        return false;
    }

    return true;
}

bool GlobalEventInput::key_up(int key)
{
    // 键盘释放
    if (!post_keyboard_event((CGKeyCode)key, false)) {
        LogError << "Failed to post keyboard up event";
        return false;
    }

    return true;
}

bool GlobalEventInput::scroll(int dx, int dy)
{
    if (!activate_window(pid_)) {
        LogWarn << "Failed to activate window, scroll may not work";
    }

    CGPoint location = CGPointZero;
    CGEventRef pos_event = CGEventCreate(nullptr);
    if (pos_event) {
        location = CGEventGetLocation(pos_event);
        CFRelease(pos_event);
    }
    else {
        LogWarn << "Failed to create position event, using default location (0, 0)";
    }

    // CGEvent wheel2: 正值向左、负值向右，与协议（正值向右）相反，需取反 dx
    CGEventRef event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, dy, -dx);
    if (!event) {
        LogError << "Failed to create scroll wheel event";
        return false;
    }

    CGEventSetLocation(event, location);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);

    return true;
}

bool GlobalEventInput::activate_window(pid_t target_pid)
{
    // 全屏情况
    if (window_id_ == 0) {
        return true;
    }

    if (target_pid <= 0) {
        LogError << "Invalid target PID: " << target_pid;
        return false;
    }

    // 使用NSRunningApplication激活应用
    // 这会将整个应用带到前台
    NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:target_pid];
    if (app) {
        [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
        return true;
    }

    return false;
}

void GlobalEventInput::update_window_info()
{
    // 全屏情况
    if (window_id_ == 0) {
        pid_ = -1;
        offset_x_ = 0;
        offset_y_ = 0;
        return;
    }

    WindowInfo info;
    if (get_window_info(window_id_, info)) {
        pid_ = info.pid;
        offset_x_ = static_cast<int>(info.bounds.origin.x);
        offset_y_ = static_cast<int>(info.bounds.origin.y);
    }
}

bool GlobalEventInput::post_mouse_event(CGEventType type, CGPoint location, CGMouseButton button)
{
    CGEventRef event = CGEventCreateMouseEvent(nullptr, type, location, button);
    if (!event) {
        LogError << "Failed to create mouse event";
        return false;
    }
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    return true;
}

bool GlobalEventInput::post_keyboard_event(CGKeyCode key_code, bool key_down)
{
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, key_code, key_down);
    if (!event) {
        LogError << "Failed to create keyboard event";
        return false;
    }
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    return true;
}

MAA_CTRL_UNIT_NS_END
