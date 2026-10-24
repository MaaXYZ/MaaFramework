#include "GestureInput.h"

#include "InputUtils.h"
#include "MaaUtils/Logger.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreFoundation/CoreFoundation.h>

#include <cmath>
#include <dlfcn.h>
#include <limits>
#include <mach/mach_time.h>
#include <mutex>
#include <tuple>

MAA_CTRL_UNIT_NS_BEGIN

GestureInput::~GestureInput()
{
    if (touching_) {
        touch_up(0);
    }
}

bool GestureInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (contact != 0 || touching_) {
        LogError << "Gesture requires contact 0 and no active touch" << VAR(contact) << VAR(touching_);
        return false;
    }

    // Preserve the nonzero begin translation from the experimental ScrollDrag tap sequence.
    // Deduct this seed from the first move so a drag retains its requested displacement.
    if (!post_gesture(kCGScrollPhaseBegan, x, y, 1, 0)) {
        return false;
    }
    latest_touch_x_ = x;
    latest_touch_y_ = y;
    touching_ = true;
    seed_pending_ = true;
    return true;
}

bool GestureInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (contact != 0 || !touching_) {
        LogError << "Gesture move requires an active touch on contact 0" << VAR(contact);
        return false;
    }

    const double dx = static_cast<double>(x) - latest_touch_x_ - (seed_pending_ ? 1 : 0);
    const double dy = static_cast<double>(y) - latest_touch_y_;
    if (!post_gesture(kCGScrollPhaseChanged, x, y, dx, dy)) {
        return false;
    }
    latest_touch_x_ = x;
    latest_touch_y_ = y;
    seed_pending_ = false;
    return true;
}

bool GestureInput::touch_up(int contact)
{
    if (contact != 0) {
        LogError << "Gesture only supports contact 0" << VAR(contact);
        return false;
    }
    // ControllerAgent may release a contact after a failed down or during cleanup.
    if (!touching_) {
        return true;
    }
    if (!post_gesture(kCGScrollPhaseEnded, latest_touch_x_, latest_touch_y_, 0, 0)) {
        return false;
    }
    touching_ = false;
    seed_pending_ = false;
    return true;
}

GestureInput::GestureRoute& GestureInput::GestureRoute::get_instance()
{
    static GestureRoute route;
    static std::once_flag initialized;
    std::call_once(initialized, [] {
        route.framework = dlopen("/System/Library/PrivateFrameworks/SkyLight.framework/SkyLight", RTLD_LAZY | RTLD_LOCAL);
        if (!route.framework) {
            return;
        }
        route.post = reinterpret_cast<GestureRoute::PostToPid>(dlsym(route.framework, "SLEventPostToPid"));
        route.set_window_location = reinterpret_cast<GestureRoute::SetWindowLocation>(dlsym(route.framework, "SLEventSetWindowLocation"));
        if (!route.set_window_location) {
            route.set_window_location = reinterpret_cast<GestureRoute::SetWindowLocation>(dlsym(RTLD_DEFAULT, "CGEventSetWindowLocation"));
        }
    });
    return route;
}

namespace GestureEventFields
{
// The private event layout; may change with macOS.
constexpr CGEventType GestureType = static_cast<CGEventType>(29);
constexpr CGEventField GestureSubtype = static_cast<CGEventField>(110);
constexpr CGEventField GesturePhase = static_cast<CGEventField>(132);
constexpr CGEventField GestureTranslationFlag = static_cast<CGEventField>(135);
constexpr CGEventField GestureDeltaX = static_cast<CGEventField>(118);
constexpr CGEventField GestureDeltaY = static_cast<CGEventField>(119);
constexpr CGEventField TargetWindow = static_cast<CGEventField>(51);
constexpr int64_t SyntheticTag = 0x4D41414745535452LL;
}

bool GestureInput::post_gesture(CGScrollPhase phase, int x, int y, double dx, double dy)
{
    using namespace GestureEventFields;
    auto& route = GestureRoute::get_instance();
    if (!route.post || !route.set_window_location) {
        LogError << "SkyLight gesture delivery is unavailable on this macOS system";
        return false;
    }
    if (dx < std::numeric_limits<int32_t>::min() || dx > std::numeric_limits<int32_t>::max() || dy < std::numeric_limits<int32_t>::min()
        || dy > std::numeric_limits<int32_t>::max()) {
        LogError << "Gesture displacement is out of range" << VAR(dx) << VAR(dy);
        return false;
    }
    WindowInfo info;
    if (!window_id_ || !get_window_info(window_id_, info) || info.pid <= 0 || info.pid != pid_ || info.bounds.size.width <= 0
        || info.bounds.size.height <= 0) {
        LogError << "Gesture target window is no longer valid" << VAR(window_id_) << VAR(pid_);
        return false;
    }

    // MaaFramework's ScreenCaptureKit output uses window points, with top-left origin.
    const CGPoint local = CGPointMake(x, y);
    const CGPoint screen = CGPointMake(info.bounds.origin.x + x, info.bounds.origin.y + y);
    EventOwner scroll(CGEventCreateScrollWheelEvent(
        nullptr, kCGScrollEventUnitPixel, 2, static_cast<int32_t>(std::lround(dy)), static_cast<int32_t>(std::lround(dx))));
    EventOwner gesture(CGEventCreate(nullptr));
    EventOwner translation(CGEventCreate(nullptr));
    if (!scroll.get() || !gesture.get() || !translation.get()) {
        LogError << "Failed to create gesture events";
        return false;
    }
    CGEventSetIntegerValueField(scroll.get(), kCGScrollWheelEventIsContinuous, 1);
    CGEventSetIntegerValueField(scroll.get(), kCGScrollWheelEventScrollPhase, phase);
    CGEventSetIntegerValueField(scroll.get(), kCGScrollWheelEventMomentumPhase, 0);
    CGEventSetType(gesture.get(), GestureType);
    CGEventSetType(translation.get(), GestureType);
    CGEventSetIntegerValueField(translation.get(), GestureSubtype, 6);
    CGEventSetIntegerValueField(translation.get(), GesturePhase, phase);
    CGEventSetIntegerValueField(translation.get(), GestureTranslationFlag, 1);
    CGEventSetDoubleValueField(translation.get(), GestureDeltaX, dx);
    CGEventSetDoubleValueField(translation.get(), GestureDeltaY, dy);

    // Preserve the recorded ScrollWheel -> Gesture -> translation ordering.
    return post_event(scroll.get(), local, screen) && post_event(gesture.get(), local, screen)
        && post_event(translation.get(), local, screen);
}

uint64_t GestureInput::event_timestamp_ns() const
{
    mach_timebase_info_data_t info { };
    mach_timebase_info(&info);
    return static_cast<uint64_t>(static_cast<__uint128_t>(mach_absolute_time()) * info.numer / info.denom);
}

bool GestureInput::post_event(CGEventRef event, CGPoint local, CGPoint screen) const
{
    using namespace GestureEventFields;
    auto& route = GestureRoute::get_instance();
    if (!event || !route.post || !route.set_window_location) {
        LogError << "SkyLight gesture delivery is unavailable on this macOS system.";
        return false;
    }
    CGEventSetLocation(event, screen);
    route.set_window_location(event, local);
    CGEventSetIntegerValueField(event, kCGEventTargetUnixProcessID, pid_);
    CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, pid_);
    CGEventSetIntegerValueField(event, TargetWindow, window_id_);
    CGEventSetIntegerValueField(event, kCGMouseEventWindowUnderMousePointer, window_id_);
    CGEventSetIntegerValueField(event, kCGMouseEventWindowUnderMousePointerThatCanHandleThisEvent, window_id_);
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, SyntheticTag);
    CGEventSetTimestamp(event, event_timestamp_ns());
    route.post(pid_, event);
    return true;
}

MAA_CTRL_UNIT_NS_END
