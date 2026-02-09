#include "GlobalEventInput.h"

#include "MaaUtils/Logger.h"

#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <tuple>

MAA_CTRL_UNIT_NS_BEGIN

MaaControllerFeature GlobalEventInput::get_features() const
{
    return MaaControllerFeature_None;
}

bool GlobalEventInput::click(int x, int y)
{
    CGPoint location = CGPointMake(x + offset_x_, y + offset_y_);

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, click may not work";
    }

    // 首先移动鼠标到目标位置
    CGEventRef mouse_move = CGEventCreateMouseEvent(
        nullptr,            // 源事件
        kCGEventMouseMoved, // 事件类型
        location,           // 位置
        kCGMouseButtonLeft  // 鼠标按钮
    );

    if (mouse_move) {
        CGEventPost(kCGHIDEventTap, mouse_move);
        CFRelease(mouse_move);
        usleep(50000); // 等待鼠标移动
    }

    // 创建鼠标按下事件
    CGEventRef click_down = CGEventCreateMouseEvent(
        nullptr,               // 源事件
        kCGEventLeftMouseDown, // 事件类型
        location,              // 位置
        kCGMouseButtonLeft     // 鼠标按钮
    );

    if (!click_down) {
        LogError << "Failed to create mouse down event";
        return false;
    }

    // 创建鼠标释放事件
    CGEventRef click_up = CGEventCreateMouseEvent(
        nullptr,             // 源事件
        kCGEventLeftMouseUp, // 事件类型
        location,            // 位置
        kCGMouseButtonLeft   // 鼠标按钮
    );

    if (!click_up) {
        LogError << "Failed to create mouse up event";
        CFRelease(click_down);
        return false;
    }

    // 使用全局事件注入
    CGEventPost(kCGHIDEventTap, click_down);
    usleep(10000); // 短暂延迟
    CGEventPost(kCGHIDEventTap, click_up);

    // 清理
    CFRelease(click_down);
    CFRelease(click_up);

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
    CGEventRef touch_down = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, start_location, kCGMouseButtonLeft);
    if (touch_down) {
        CGEventPost(kCGHIDEventTap, touch_down);
        CFRelease(touch_down);
    }

    usleep(50000); // 短暂延迟

    // 移动到终点
    CGEventRef touch_move = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDragged, end_location, kCGMouseButtonLeft);
    if (touch_move) {
        CGEventPost(kCGHIDEventTap, touch_move);
        CFRelease(touch_move);
    }

    usleep(duration * 1000); // 等待指定的持续时间

    // 释放
    CGEventRef touch_up = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, end_location, kCGMouseButtonLeft);
    if (touch_up) {
        CGEventPost(kCGHIDEventTap, touch_up);
        CFRelease(touch_up);
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

    CGEventRef touch_down = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, location, kCGMouseButtonLeft);
    if (touch_down) {
        CGEventPost(kCGHIDEventTap, touch_down);
        CFRelease(touch_down);
        return true;
    }

    return false;
}

bool GlobalEventInput::touch_move(int contact, int x, int y, int pressure)
{
    (void)pressure;
    if (contact != 0) {
        LogWarn << "Only contact 0 is supported for macOS controller";
        return false;
    }

    CGPoint location = CGPointMake(x + offset_x_, y + offset_y_);

    CGEventRef touch_move = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDragged, location, kCGMouseButtonLeft);
    if (touch_move) {
        CGEventPost(kCGHIDEventTap, touch_move);
        CFRelease(touch_move);
        return true;
    }

    return false;
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

    CGEventRef touch_up = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, location, kCGMouseButtonLeft);
    if (touch_up) {
        CGEventPost(kCGHIDEventTap, touch_up);
        CFRelease(touch_up);
        return true;
    }

    return false;
}

bool GlobalEventInput::click_key(int key)
{
    CGKeyCode key_code = (CGKeyCode)key;

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, click_key may not work";
    }

    // 创建键盘按下事件
    CGEventRef key_down = CGEventCreateKeyboardEvent(nullptr, key_code, true);
    if (key_down) {
        CGEventPost(kCGHIDEventTap, key_down);
        CFRelease(key_down);
    }

    usleep(10000); // 短暂延迟

    // 创建键盘释放事件
    CGEventRef key_up = CGEventCreateKeyboardEvent(nullptr, key_code, false);
    if (key_up) {
        CGEventPost(kCGHIDEventTap, key_up);
        CFRelease(key_up);
    }

    return true;
}

bool GlobalEventInput::input_text(const std::string& text)
{
    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, input_text may not work";
    }

    // CGKeyCode 映射表 (基于标准美式键盘布局)
    static std::map<char, CGKeyCode> key_map
        = { { 'a', 0 },   { 'b', 11 }, { 'c', 8 },  { 'd', 2 },  { 'e', 14 },  { 'f', 3 },  { 'g', 5 },  { 'h', 4 },  { 'i', 34 },
            { 'j', 38 },  { 'k', 40 }, { 'l', 37 }, { 'm', 46 }, { 'n', 45 },  { 'o', 31 }, { 'p', 35 }, { 'q', 12 }, { 'r', 15 },
            { 's', 1 },   { 't', 17 }, { 'u', 32 }, { 'v', 9 },  { 'w', 13 },  { 'x', 7 },  { 'y', 16 }, { 'z', 6 },  { 'A', 0 },
            { 'B', 11 },  { 'C', 8 },  { 'D', 2 },  { 'E', 14 }, { 'F', 3 },   { 'G', 5 },  { 'H', 4 },  { 'I', 34 }, { 'J', 38 },
            { 'K', 40 },  { 'L', 37 }, { 'M', 46 }, { 'N', 45 }, { 'O', 31 },  { 'P', 35 }, { 'Q', 12 }, { 'R', 15 }, { 'S', 1 },
            { 'T', 17 },  { 'U', 32 }, { 'V', 9 },  { 'W', 13 }, { 'X', 7 },   { 'Y', 16 }, { 'Z', 6 },  { ' ', 49 }, { '1', 18 },
            { '2', 19 },  { '3', 20 }, { '4', 21 }, { '5', 23 }, { '6', 22 },  { '7', 26 }, { '8', 28 }, { '9', 25 }, { '0', 29 },
            { '!', 18 },  { '@', 19 }, { '#', 20 }, { '$', 21 }, { '%', 23 },  { '^', 22 }, { '&', 26 }, { '*', 28 }, { '(', 25 },
            { ')', 29 },  { '-', 27 }, { '_', 27 }, { '=', 24 }, { '+', 24 },  { '[', 33 }, { ']', 30 }, { '{', 33 }, { '}', 30 },
            { '\\', 42 }, { '|', 42 }, { ';', 41 }, { ':', 41 }, { '\'', 39 }, { '"', 39 }, { ',', 43 }, { '<', 43 }, { '.', 47 },
            { '>', 47 },  { '/', 44 }, { '?', 44 }, { '`', 50 }, { '~', 50 } };

    for (char c : text) {
        auto it = key_map.find(c);
        CGKeyCode key_code;
        bool need_shift = false;

        if (it != key_map.end()) {
            key_code = it->second;
            // 检查是否需要Shift（通过比较小写和大写版本的键码）
            if (isupper(c)) {
                auto lower_it = key_map.find(tolower(c));
                if (lower_it != key_map.end() && lower_it->second == key_code) {
                    need_shift = true;
                }
            }
        }
        else {
            LogWarn << "Unsupported character: " << std::string(1, c) << " (code: " << (int)(unsigned char)c << ")";
            continue;
        }

        // 如果是大写字母，先按下Shift
        if (need_shift) {
            CGEventRef shift_down = CGEventCreateKeyboardEvent(nullptr, 56, true); // 左Shift键
            if (shift_down) {
                CGEventPost(kCGHIDEventTap, shift_down);
                CFRelease(shift_down);
            }
            usleep(10000);
        }

        // 按下键
        CGEventRef key_down = CGEventCreateKeyboardEvent(nullptr, key_code, true);
        if (key_down) {
            CGEventPost(kCGHIDEventTap, key_down);
            CFRelease(key_down);
        }
        usleep(10000);

        // 释放键
        CGEventRef key_up = CGEventCreateKeyboardEvent(nullptr, key_code, false);
        if (key_up) {
            CGEventPost(kCGHIDEventTap, key_up);
            CFRelease(key_up);
        }
        usleep(10000);

        // 释放Shift
        if (need_shift) {
            CGEventRef shift_up = CGEventCreateKeyboardEvent(nullptr, 56, false);
            if (shift_up) {
                CGEventPost(kCGHIDEventTap, shift_up);
                CFRelease(shift_up);
            }
            usleep(10000);
        }

        // 字符间延迟
        usleep(50000);
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
    CGEventRef key_down = CGEventCreateKeyboardEvent(nullptr, key_code, true);
    if (key_down) {
        CGEventPost(kCGHIDEventTap, key_down);
        CFRelease(key_down);
        return true;
    }

    return false;
}

bool GlobalEventInput::key_up(int key)
{
    CGKeyCode key_code = (CGKeyCode)key;

    // 先激活窗口
    if (!activate_window(pid_)) {
        LogWarn << "Warning: Failed to activate window, key_up may not work";
    }

    // 创建键盘释放事件
    CGEventRef key_up = CGEventCreateKeyboardEvent(nullptr, key_code, false);
    if (key_up) {
        CGEventPost(kCGHIDEventTap, key_up);
        CFRelease(key_up);
        return true;
    }

    return false;
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
    if (target_pid <= 0) {
        LogError << "Invalid target PID: " << target_pid;
        return false;
    }

    // 使用NSRunningApplication激活应用
    // 这会将整个应用带到前台
    NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:target_pid];
    if (app) {
        [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
        usleep(200000); // 等待窗口激活
        return true;
    }

    return false;
}

std::tuple<pid_t, int, int> GlobalEventInput::get_window_info(uint32_t window_id)
{
    pid_t pid = -1;
    int offset_x = 0;
    int offset_y = 0;

    // 获取窗口信息
    CFArrayRef window_list = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, window_id);

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

MAA_CTRL_UNIT_NS_END
