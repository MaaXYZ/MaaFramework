#include "MaaToolkit/MacOS/MaaToolkitMacOS.h"

#import <Cocoa/Cocoa.h>

MaaBool MaaToolkitMacOSCheckPermission(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return CGPreflightScreenCaptureAccess();
    case MaaMacOSPermissionAccessibility:
        return AXIsProcessTrusted();
    }
    return false;
}

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

MaaBool MaaToolkitMacOSRequestPermission(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return CGRequestScreenCaptureAccess();
    case MaaMacOSPermissionAccessibility:
        return requestAccessibility();
    }
    return false;
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

MaaBool MaaToolkitMacOSRevealPermissionSettings(MaaMacOSPermission perm)
{
    switch (perm) {
    case MaaMacOSPermissionScreenCapture:
        return openUrl(@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture");
    case MaaMacOSPermissionAccessibility:
        return openUrl(@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility");
    }
    return false;
}
