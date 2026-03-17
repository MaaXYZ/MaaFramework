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
    return MaaControllerFeature_None;
}

bool GlobalEventInput::click(int x, int y)
{
    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, click may not work";
    }

    // 计算点击位置的绝对坐标
    CGPoint location = CGPointMake(x + offset_x_, y + offset_y_);

    // 移动鼠标到目标位置
    if (!post_mouse_event(kCGEventMouseMoved, location)) {
        LogError << "Failed to post mouse move event";
        return false;
    }

    usleep(10000);

    // 鼠标按下
    if (!post_mouse_event(kCGEventLeftMouseDown, location)) {
        LogError << "Failed to post mouse down event";
        return false;
    }

    usleep(10000);

    // 鼠标释放
    if (!post_mouse_event(kCGEventLeftMouseUp, location)) {
        LogError << "Failed to post mouse up event";
        return false;
    }

    return true;
}

bool GlobalEventInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    // 简单的滑动实现：从起点移动到终点

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, swipe may not work";
    }

    CGPoint start_location = CGPointMake(x1 + offset_x_, y1 + offset_y_);
    CGPoint end_location = CGPointMake(x2 + offset_x_, y2 + offset_y_);

    // 按下
    if (!post_mouse_event(kCGEventLeftMouseDown, start_location, kCGMouseButtonLeft)) {
        LogError << "Failed to post mouse down event";
        return false;
    }
    usleep(10000);

    // 移动到终点
    if (!post_mouse_event(kCGEventLeftMouseDragged, end_location, kCGMouseButtonLeft)) {
        LogError << "Failed to post mouse dragged event";
        return false;
    }
    usleep(duration * 1000); // 等待指定的持续时间

    // 释放
    if (!post_mouse_event(kCGEventLeftMouseUp, end_location, kCGMouseButtonLeft)) {
        LogError << "Failed to post mouse up event";
        return false;
    }

    return true;
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

    CGPoint location = CGPointMake(x + offset_x_, y + offset_y_);

    if (!post_mouse_event(kCGEventLeftMouseDown, location, kCGMouseButtonLeft)) {
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

    CGPoint location = CGPointMake(x + offset_x_, y + offset_y_);

    if (!post_mouse_event(kCGEventLeftMouseDragged, location, kCGMouseButtonLeft)) {
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

bool GlobalEventInput::click_key(int key)
{
    CGKeyCode key_code = (CGKeyCode)key;

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, click_key may not work";
    }

    // 键盘按下
    if (!post_keyboard_event(key_code, true)) {
        LogError << "Failed to post keyboard down event";
        return false;
    }

    usleep(10000);

    // 键盘释放
    if (!post_keyboard_event(key_code, false)) {
        LogError << "Failed to post keyboard up event";
        return false;
    }

    return true;
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
    CGKeyCode key_code = (CGKeyCode)key;

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, key_down may not work";
    }

    // 创建键盘按下事件
    if (!post_keyboard_event(key_code, true)) {
        LogError << "Failed to post keyboard down event";
        return false;
    }

    return true;
}

bool GlobalEventInput::key_up(int key)
{
    CGKeyCode key_code = (CGKeyCode)key;

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, key_up may not work";
    }

    // 创建键盘释放事件
    if (!post_keyboard_event(key_code, false)) {
        LogError << "Failed to post keyboard up event";
        return false;
    }

    return true;
}

bool GlobalEventInput::scroll(int dx, int dy)
{
    (void)dx;
    (void)dy;
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

std::tuple<pid_t, int, int> GlobalEventInput::get_window_info()
{
    pid_t pid = -1;
    int offset_x = 0;
    int offset_y = 0;

    // 全屏情况
    if (window_id_ == 0) {
        return { pid, offset_x, offset_y };
    }

    // 获取窗口信息
    CFArrayRef window_list = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, window_id_);

    if (!window_list || CFArrayGetCount(window_list) == 0) {
        if (window_list) {
            CFRelease(window_list);
        }
        return { pid, offset_x, offset_y };
    }

    CFDictionaryRef window_info = (CFDictionaryRef)CFArrayGetValueAtIndex(window_list, 0);

    // 获取进程PID
    CFNumberRef pid_ref = (CFNumberRef)CFDictionaryGetValue(window_info, kCGWindowOwnerPID);
    if (pid_ref) {
        CFNumberGetValue(pid_ref, kCFNumberIntType, &pid);
    }

    // 获取窗口边界
    CFDictionaryRef bounds_ref = (CFDictionaryRef)CFDictionaryGetValue(window_info, kCGWindowBounds);
    if (bounds_ref) {
        CGRect bounds;
        if (CGRectMakeWithDictionaryRepresentation(bounds_ref, &bounds)) {
            offset_x = static_cast<int>(bounds.origin.x);
            offset_y = static_cast<int>(bounds.origin.y);
        }
    }

    CFRelease(window_list);
    return { pid, offset_x, offset_y };
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
