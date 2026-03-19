#pragma once

#include "Common/Conf.h"

#include <ApplicationServices/ApplicationServices.h>

MAA_CTRL_UNIT_NS_BEGIN

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
