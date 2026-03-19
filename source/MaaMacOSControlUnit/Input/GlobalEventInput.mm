#include "GlobalEventInput.h"

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
    if (contact != 0) {
        LogWarn << "Only contact 0 is supported for macOS controller";
        return false;
    }

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, touch may not work";
    }

    if (!post_mouse_event(kCGEventLeftMouseDown, CGPointMake(x + offset_x_, y + offset_y_), kCGMouseButtonLeft)) {
        LogError << "Failed to post mouse down event";
        return false;
    }

    return true;
}

bool GlobalEventInput::touch_move(int contact, int x, int y, int pressure)
{
    (void)pressure;
    if (contact != 0) {
        LogWarn << "Only contact 0 is supported for macOS controller";
        return false;
    }

    if (!post_mouse_event(kCGEventLeftMouseDragged, CGPointMake(x + offset_x_, y + offset_y_), kCGMouseButtonLeft)) {
        LogError << "Failed to post mouse dragged event";
        return false;
    }

    return true;
}

bool GlobalEventInput::touch_up(int contact)
{
    if (contact != 0) {
        LogWarn << "Only contact 0 is supported for macOS controller";
        return false;
    }

    // 获取当前鼠标位置作为释放位置
    CGEventRef current_event = CGEventCreate(nullptr);
    CGPoint location = CGEventGetLocation(current_event);
    CFRelease(current_event);

    if (!post_mouse_event(kCGEventLeftMouseUp, location, kCGMouseButtonLeft)) {
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
    NSString* ns_text = [NSString stringWithUTF8String:text.c_str()];
    if (!ns_text) {
        LogError << "Failed to convert text to NSString: " << text;
        return false;
    }

    NSUInteger len = [ns_text length];
    std::vector<UniChar> chars(len);
    [ns_text getCharacters:chars.data() range:NSMakeRange(0, len)];

    // key_down + key_up 各发一次，接收方通常只处理 key_down
    for (bool is_down : { true, false }) {
        CGEventRef event = CGEventCreateKeyboardEvent(nullptr, 0, is_down);
        if (!event) {
            LogError << "Failed to create keyboard event";
            return false;
        }
        CGEventKeyboardSetUnicodeString(event, len, chars.data());
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
    // 使用当前系统光标位置作为滚动目标点
    CGEventRef pos_event = CGEventCreate(nullptr);
    CGPoint location = CGEventGetLocation(pos_event);
    CFRelease(pos_event);

    // wheel1: 垂直滚动（正值向上，负值向下），wheel2: 水平滚动（正值向左，负值向右）
    CGEventRef event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, dy, dx);
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

    // 获取窗口信息
    CFArrayRef window_list = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, window_id_);

    if (!window_list || CFArrayGetCount(window_list) == 0) {
        if (window_list) {
            CFRelease(window_list);
        }
        return;
    }

    CFDictionaryRef window_info = (CFDictionaryRef)CFArrayGetValueAtIndex(window_list, 0);

    // 获取进程PID
    CFNumberRef pid_ref = (CFNumberRef)CFDictionaryGetValue(window_info, kCGWindowOwnerPID);
    if (pid_ref) {
        CFNumberGetValue(pid_ref, kCFNumberIntType, &pid_);
    }

    // 获取窗口位置
    CFDictionaryRef bounds_ref = (CFDictionaryRef)CFDictionaryGetValue(window_info, kCGWindowBounds);
    if (bounds_ref) {
        CGRect bounds;
        if (CGRectMakeWithDictionaryRepresentation(bounds_ref, &bounds)) {
            offset_x_ = static_cast<int>(bounds.origin.x);
            offset_y_ = static_cast<int>(bounds.origin.y);
        }
    }

    CFRelease(window_list);
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
