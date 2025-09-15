#include "CGEventInput.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"

#import <Cocoa/Cocoa.h>

MAA_CTRL_UNIT_NS_BEGIN

CGEventInput::CGEventInput(CGWindowID windowId)
    : window_id_(windowId)
{
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);

    NSArray* windows = CFBridgingRelease(windowList);
    for (NSDictionary* window in windows) {
        NSNumber* windowIDNumber = window[(id)kCGWindowNumber];
        if (windowIDNumber && [windowIDNumber unsignedIntValue] == windowId) {
            NSNumber* ownerPID = window[(id)kCGWindowOwnerPID];
            pid_ = [ownerPID integerValue];
            break;
        }
    }

    CFRelease(windowList);
}

bool CGEventInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    auto downEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                        location:CGPointMake(x, y)
                                   modifierFlags:0
                                       timestamp:[NSDate timeIntervalSinceReferenceDate]
                                    windowNumber:window_id_
                                         context:nil
                                     eventNumber:0
                                      clickCount:1
                                        pressure:0];
    CGEventPostToPid(pid_, [downEvent CGEvent]);

    auto upEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                                      location:CGPointMake(x, y)
                                 modifierFlags:0
                                     timestamp:[NSDate timeIntervalSinceReferenceDate]
                                  windowNumber:window_id_
                                       context:nil
                                   eventNumber:0
                                    clickCount:1
                                      pressure:0];
    CGEventPostToPid(pid_, [upEvent CGEvent]);

    return true;
}

bool CGEventInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    auto start = std::chrono::steady_clock::now();
    auto now = start;

    auto downEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                        location:CGPointMake(x1, y1)
                                   modifierFlags:0
                                       timestamp:[NSDate timeIntervalSinceReferenceDate]
                                    windowNumber:window_id_
                                         context:nil
                                     eventNumber:0
                                      clickCount:1
                                        pressure:0];
    CGEventPostToPid(pid_, [downEvent CGEvent]);

    constexpr double kInterval = 10; // ms
    const double steps = duration / kInterval;
    const double x_step_len = (x2 - x1) / steps;
    const double y_step_len = (y2 - y1) / steps;
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    for (int i = 0; i < steps; ++i) {
        int tx = static_cast<int>(x1 + i * x_step_len);
        int ty = static_cast<int>(y1 + i * y_step_len);
        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();

        auto moveEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                            location:CGPointMake(tx, ty)
                                       modifierFlags:0
                                           timestamp:[NSDate timeIntervalSinceReferenceDate]
                                        windowNumber:window_id_
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:0];
        CGEventPostToPid(pid_, [moveEvent CGEvent]);
    }

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();

    auto moveEventEnd = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                           location:CGPointMake(x2, y2)
                                      modifierFlags:0
                                          timestamp:[NSDate timeIntervalSinceReferenceDate]
                                       windowNumber:window_id_
                                            context:nil
                                        eventNumber:0
                                         clickCount:1
                                           pressure:0];
    CGEventPostToPid(pid_, [moveEventEnd CGEvent]);

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();

    auto upEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                                      location:CGPointMake(x2, y2)
                                 modifierFlags:0
                                     timestamp:[NSDate timeIntervalSinceReferenceDate]
                                  windowNumber:window_id_
                                       context:nil
                                   eventNumber:0
                                    clickCount:1
                                      pressure:0];
    CGEventPostToPid(pid_, [upEvent CGEvent]);

    return true;
}

bool CGEventInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = contact;
    std::ignore = pressure;

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    auto downEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                        location:CGPointMake(x, y)
                                   modifierFlags:0
                                       timestamp:[NSDate timeIntervalSinceReferenceDate]
                                    windowNumber:window_id_
                                         context:nil
                                     eventNumber:0
                                      clickCount:1
                                        pressure:0];
    CGEventPostToPid(pid_, [downEvent CGEvent]);

    return true;
}

bool CGEventInput::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = contact;
    std::ignore = pressure;

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    auto moveEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                        location:CGPointMake(x, y)
                                   modifierFlags:0
                                       timestamp:[NSDate timeIntervalSinceReferenceDate]
                                    windowNumber:window_id_
                                         context:nil
                                     eventNumber:0
                                      clickCount:1
                                        pressure:0];
    CGEventPostToPid(pid_, [moveEvent CGEvent]);

    cache_x_ = x;
    cache_y_ = y;

    return true;
}

bool CGEventInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

    std::ignore = contact;

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    auto upEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                                      location:CGPointMake(cache_x_, cache_y_)
                                 modifierFlags:0
                                     timestamp:[NSDate timeIntervalSinceReferenceDate]
                                  windowNumber:window_id_
                                       context:nil
                                   eventNumber:0
                                    clickCount:1
                                      pressure:0];
    CGEventPostToPid(pid_, [upEvent CGEvent]);

    return true;
}

bool CGEventInput::click_key(int key)
{
    LogInfo << VAR(key);

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    auto downEvent = CGEventCreateKeyboardEvent(NULL, 0, YES);
    CGEventPostToPid(pid_, downEvent);

    auto upEvent = CGEventCreateKeyboardEvent(NULL, 0, NO);
    CGEventPostToPid(pid_, upEvent);
    // auto downEvent = [NSEvent keyEventWithType:NSEventTypeKeyDown
    //                                   location:CGPointMake(0, 0)
    //                              modifierFlags:0
    //                                  timestamp:[NSDate timeIntervalSinceReferenceDate]
    //                               windowNumber:window_id_
    //                                    context:nil
    //                                 characters:@""
    //                charactersIgnoringModifiers:@""
    //                                  isARepeat:NO
    //                                    keyCode:key];
    // CGEventPostToPid(pid_, [downEvent CGEvent]);

    // auto upEvent = [NSEvent keyEventWithType:NSEventTypeKeyUp
    //                                 location:CGPointMake(0, 0)
    //                            modifierFlags:0
    //                                timestamp:[NSDate timeIntervalSinceReferenceDate]
    //                             windowNumber:window_id_
    //                                  context:nil
    //                               characters:@""
    //              charactersIgnoringModifiers:@""
    //                                isARepeat:NO
    //                                  keyCode:key];
    // CGEventPostToPid(pid_, [upEvent CGEvent]);

    return true;
}

bool CGEventInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!window_id_) {
        LogError << "window_id_ is 0";
        return false;
    }

    if (!pid_) {
        LogError << "pid_ is 0";
        return false;
    }

    LogError << "input_text not supported";

    return false;
}

MAA_CTRL_UNIT_NS_END
