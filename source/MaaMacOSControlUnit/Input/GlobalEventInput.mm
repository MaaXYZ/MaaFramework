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
    (void)pressure;

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
    (void)pressure;

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
    CGEventRef current_event = CGEventCreate(nullptr);
    CGPoint location = CGEventGetLocation(current_event);
    CFRelease(current_event);

    if (!post_mouse_event(info.event_type, location, info.mouse_button)) {
        LogError << "Failed to post mouse up event";
        return false;
    }

    return true;
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
    std::ignore = dx;
    std::ignore = dy;
    LogWarn << "scroll not supported on macOS controller";
    return false;
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
    if (event) {
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
        return true;
    }
    return false;
}

bool GlobalEventInput::post_keyboard_event(CGKeyCode key_code, bool key_down)
{
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, key_code, key_down);
    if (event) {
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
        return true;
    }
    return false;
}

MAA_CTRL_UNIT_NS_END
