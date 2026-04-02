#include "MacOSPermission.h"

#import <Cocoa/Cocoa.h>

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

static bool requestAccessibility()
{
    const void* keys[] = { kAXTrustedCheckOptionPrompt };
    const void* values[] = { kCFBooleanTrue };
    CFDictionaryRef options
        = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    Boolean trusted = AXIsProcessTrustedWithOptions(options);
    if (options) {
        CFRelease(options);
    }
    return trusted;
}

static bool openUrl(NSString* urlString)
{
    @autoreleasepool {
        NSURL* url = [NSURL URLWithString:urlString];
        if (!url) {
            return false;
        }
        NSWorkspace* ws = [NSWorkspace sharedWorkspace];
        return [ws openURL:url];
    }
}

bool MacOSCheckPermission(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return CGPreflightScreenCaptureAccess();
    case MaaMacOSPermissionAccessibility:
        return AXIsProcessTrusted();
    }
    LogWarn << "Unknown permission" << VAR(perm);
    return false;
}

bool MacOSRequestPermission(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return CGRequestScreenCaptureAccess();
    case MaaMacOSPermissionAccessibility:
        return requestAccessibility();
    }
    LogWarn << "Unknown permission" << VAR(perm);
    return false;
}

bool MacOSRevealPermissionSettings(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return openUrl(@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture");
    case MaaMacOSPermissionAccessibility:
        return openUrl(@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility");
    }
    LogWarn << "Unknown permission" << VAR(perm);
    return false;
}

MAA_TOOLKIT_NS_END
