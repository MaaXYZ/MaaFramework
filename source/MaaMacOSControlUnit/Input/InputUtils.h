#pragma once

#include "Common/Conf.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Foundation/Foundation.h>
#include <vector>

MAA_CTRL_UNIT_NS_BEGIN

struct WindowInfo
{
    pid_t pid = -1;
    CGRect bounds = CGRectZero;
};

// 查询指定窗口的 PID 和边界矩形，返回 false 表示查询失败
inline bool get_window_info(uint32_t window_id, WindowInfo& out)
{
    CFArrayRef window_list = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, window_id);

    if (!window_list || CFArrayGetCount(window_list) == 0) {
        if (window_list) {
            CFRelease(window_list);
        }
        return false;
    }

    CFDictionaryRef info = (CFDictionaryRef)CFArrayGetValueAtIndex(window_list, 0);

    CFNumberRef pid_ref = (CFNumberRef)CFDictionaryGetValue(info, kCGWindowOwnerPID);
    if (pid_ref) {
        CFNumberGetValue(pid_ref, kCFNumberIntType, &out.pid);
    }

    CFDictionaryRef bounds_ref = (CFDictionaryRef)CFDictionaryGetValue(info, kCGWindowBounds);
    if (bounds_ref) {
        CGRectMakeWithDictionaryRepresentation(bounds_ref, &out.bounds);
    }

    CFRelease(window_list);
    return true;
}

// 将 UTF-8 文本转换为 UTF-16 UniChar 数组
inline bool text_to_unichars(const std::string& text, std::vector<UniChar>& out)
{
    NSString* ns_text = [NSString stringWithUTF8String:text.c_str()];
    if (!ns_text) {
        return false;
    }

    NSUInteger len = [ns_text length];
    out.resize(len);
    [ns_text getCharacters:out.data() range:NSMakeRange(0, len)];
    return true;
}

struct MouseEventInfo
{
    CGEventType event_type;
    CGMouseButton mouse_button;
};

// 将 contact ID 转换为鼠标按下事件信息
// contact: 0=左键, 1=右键, 2=中键
inline bool contact_to_mouse_down_info(int contact, MouseEventInfo& info)
{
    switch (contact) {
    case 0:
        info = { kCGEventLeftMouseDown, kCGMouseButtonLeft };
        return true;
    case 1:
        info = { kCGEventRightMouseDown, kCGMouseButtonRight };
        return true;
    case 2:
        info = { kCGEventOtherMouseDown, kCGMouseButtonCenter };
        return true;
    default:
        return false;
    }
}

// 将 contact ID 转换为鼠标拖拽事件信息
// contact: 0=左键拖拽, 1=右键拖拽, 2=中键拖拽
inline bool contact_to_mouse_move_info(int contact, MouseEventInfo& info)
{
    switch (contact) {
    case 0:
        info = { kCGEventLeftMouseDragged, kCGMouseButtonLeft };
        return true;
    case 1:
        info = { kCGEventRightMouseDragged, kCGMouseButtonRight };
        return true;
    case 2:
        info = { kCGEventOtherMouseDragged, kCGMouseButtonCenter };
        return true;
    default:
        return false;
    }
}

// 将 contact ID 转换为鼠标抬起事件信息
// contact: 0=左键, 1=右键, 2=中键
inline bool contact_to_mouse_up_info(int contact, MouseEventInfo& info)
{
    switch (contact) {
    case 0:
        info = { kCGEventLeftMouseUp, kCGMouseButtonLeft };
        return true;
    case 1:
        info = { kCGEventRightMouseUp, kCGMouseButtonRight };
        return true;
    case 2:
        info = { kCGEventOtherMouseUp, kCGMouseButtonCenter };
        return true;
    default:
        return false;
    }
}

MAA_CTRL_UNIT_NS_END
