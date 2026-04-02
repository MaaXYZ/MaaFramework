#include "PostToPidInput.h"

#include "InputUtils.h"
#include "MaaUtils/Logger.h"

#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <tuple>
#include <unistd.h>
#include <vector>

MAA_CTRL_UNIT_NS_BEGIN

MaaControllerFeature PostToPidInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 touch_down/touch_up 替代 click/swipe
bool PostToPidInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool PostToPidInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool PostToPidInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    MouseEventInfo info;
    if (!contact_to_mouse_down_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    if (!post_mouse_event(info.event_type, x, y)) {
        LogError << "Failed to post mouse down event";
        return false;
    }

    latest_touch_x_ = x;
    latest_touch_y_ = y;

    return true;
}

bool PostToPidInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    MouseEventInfo info;
    if (!contact_to_mouse_move_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    if (!post_mouse_event(info.event_type, x, y)) {
        LogError << "Failed to post mouse dragged event";
        return false;
    }

    latest_touch_x_ = x;
    latest_touch_y_ = y;

    return true;
}

bool PostToPidInput::touch_up(int contact)
{
    MouseEventInfo info;
    if (!contact_to_mouse_up_info(contact, info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    if (!post_mouse_event(info.event_type, latest_touch_x_, latest_touch_y_)) {
        LogError << "Failed to post mouse up event";
        return false;
    }

    return true;
}

// get_features() 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 key_down/key_up 替代 click_key
bool PostToPidInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool PostToPidInput::input_text(const std::string& text)
{
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
        CGEventPostToPid(pid_, event);
        CFRelease(event);
        usleep(10000);
    }

    return true;
}

bool PostToPidInput::key_down(int key)
{
    // 键盘按下
    if (!post_keyboard_event((CGKeyCode)key, true)) {
        LogError << "Failed to post keyboard down event";
        return false;
    }

    return true;
}

bool PostToPidInput::key_up(int key)
{
    // 键盘释放
    if (!post_keyboard_event((CGKeyCode)key, false)) {
        LogError << "Failed to post keyboard up event";
        return false;
    }

    return true;
}

bool PostToPidInput::scroll(int dx, int dy)
{
    NSPoint location = NSMakePoint(latest_touch_x_, window_h_ - latest_touch_y_);
    NSEvent* nsEvent0 = [NSEvent mouseEventWithType:NSEventTypeMouseMoved
                                           location:location
                                      modifierFlags:0
                                          timestamp:[NSDate timeIntervalSinceReferenceDate]
                                       windowNumber:window_id_
                                            context:nil
                                        eventNumber:0
                                         clickCount:1
                                           pressure:0];
    if (!nsEvent0) {
        LogError << "Failed to create scroll nsEvent0";
        return false;
    }
    [nsEvent0 setValue:@(NSEventTypeScrollWheel) forKey:@"type"];
    CGEventRef event0 = [nsEvent0 CGEvent];
    if (!event0) {
        LogError << "Failed to create scroll event0";
        return false;
    }

    // CGEvent wheel2: 正值向左、负值向右，与协议（正值向右）相反，需取反 dx
    CGEventRef event1 = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, dy, -dx);
    if (!event1) {
        LogError << "Failed to create scroll event1";
        return false;
    }

    CGEventPostToPid(pid_, event0);
    CGEventPostToPid(pid_, event1);

    CFRelease(event1);

    return true;
}

void PostToPidInput::update_window_info()
{
    WindowInfo info;
    if (get_window_info(window_id_, info)) {
        pid_ = info.pid;
        window_w_ = static_cast<int>(info.bounds.size.width);
        window_h_ = static_cast<int>(info.bounds.size.height);
    }
}

bool PostToPidInput::post_mouse_event(CGEventType type, int x, int y)
{
    // NSEvent 使用 AppKit 坐标系（原点在主屏左下角，Y 向上），
    // 而传入的坐标是 CGEvent 坐标系（原点在左上角，Y 向下），需要翻转 Y。
    NSPoint appkitLocation = NSMakePoint(x, window_h_ - y);

    NSEvent* nsEvent = [NSEvent mouseEventWithType:(NSEventType)type
                                          location:appkitLocation
                                     modifierFlags:0
                                         timestamp:[NSDate timeIntervalSinceReferenceDate]
                                      windowNumber:window_id_
                                           context:nil
                                       eventNumber:0
                                        clickCount:1
                                          pressure:0];
    if (!nsEvent) {
        LogError << "Failed to create mouse nsEvent";
        return false;
    }
    CGEventRef cgEvent = [nsEvent CGEvent];
    if (!cgEvent) {
        LogError << "Failed to create mouse cgEvent";
        return false;
    }
    CGEventPostToPid(pid_, cgEvent);
    return true;
}

bool PostToPidInput::post_keyboard_event(CGKeyCode key_code, bool key_down)
{
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, key_code, key_down);
    if (!event) {
        LogError << "Failed to create keyboard event";
        return false;
    }
    CGEventPostToPid(pid_, event);
    CFRelease(event);
    return true;
}

MAA_CTRL_UNIT_NS_END
