#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

#include "DesktopWindowMacOSFinder.h"

#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowMacOSFinder::find_all() const
{
    std::vector<DesktopWindow> windows;

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);

    NSArray* windowsInfo = CFBridgingRelease(windowList);
    for (NSDictionary* window in windowsInfo) {
        if (!window[(id)kCGWindowLayer] || [window[(id)kCGWindowLayer] integerValue] != 0) {
            continue;
        }

        id hwnd = window[(id)kCGWindowNumber];
        if (!hwnd) {
            continue;
        }

        windows.push_back({});
        auto& info = windows.back();

        info.hwnd = reinterpret_cast<void*>(static_cast<CGWindowID>([hwnd unsignedIntValue]));

        NSString* window_name = window[(id)kCGWindowName];
        info.window_name = window_name ? [window_name UTF8String] : "";

        NSString* app_name = window[(id)kCGWindowOwnerName];
        info.class_name = app_name ? [app_name UTF8String] : "";
    }

    CFRelease(windowList);

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif // __APPLE__
