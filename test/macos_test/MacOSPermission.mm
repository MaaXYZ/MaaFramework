
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#include "./MacOSPermission.h"

bool check_screen_recording_permission(void)
{
    if (@available(macOS 10.15, *)) {
        return CGPreflightScreenCaptureAccess();
    }
    return true;
}

bool request_screen_recording_permission(void)
{
    if (@available(macOS 10.15, *)) {
        return CGRequestScreenCaptureAccess();
    }
    return true; // Assume granted on older macOS
}

// 打开“屏幕录制”隐私设置页，方便用户手动授予权限
bool open_screen_recording_privacy_settings(void)
{
    @autoreleasepool {
        NSURL* url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"];
        if (!url) {
            return false;
        }
        NSWorkspace* ws = [NSWorkspace sharedWorkspace];
        BOOL opened = [ws openURL:url];
        return opened ? true : false;
    }
}

bool check_accessibility_permission(void)
{
    if (@available(macOS 10.9, *)) {
        return AXIsProcessTrusted();
    }
    return true;
}

bool request_accessibility_permission(void)
{
    // 使用带提示的方式请求辅助功能权限（AXIsProcessTrustedWithOptions）
    // 如果系统支持并允许，此调用会触发系统弹窗（设置 kAXTrustedCheckOptionPrompt = true）
    if (@available(macOS 10.9, *)) {
        CFStringRef key = kAXTrustedCheckOptionPrompt;
        const void *keys[] = { key };
        const void *values[] = { kCFBooleanTrue };
        CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                                     &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        Boolean trusted = AXIsProcessTrustedWithOptions(options);
        if (options) CFRelease(options);
        return trusted ? true : false;
    }
    return true;
}

// 打开“辅助功能”隐私设置页，方便用户手动授予权限
bool open_accessibility_privacy_settings(void)
{
    @autoreleasepool {
        // 在大多数 macOS 版本中，此 URL 会打开“隐私与安全性 -> 辅助功能”设置
        NSURL* url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
        if (!url) {
            return false;
        }
        NSWorkspace* ws = [NSWorkspace sharedWorkspace];
        BOOL opened = [ws openURL:url];
        return opened ? true : false;
    }
}
