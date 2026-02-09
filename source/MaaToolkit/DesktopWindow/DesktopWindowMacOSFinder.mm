#ifdef __APPLE__

#include "DesktopWindowMacOSFinder.h"

#include "MaaUtils/Logger.h"

#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowMacOSFinder::find_all() const
{
    LogFunc;

    std::vector<DesktopWindow> windows;

    // 检查macOS版本
    if (@available(macOS 12.3, *)) {
        // 使用GCD信号量进行同步
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        __block std::vector<DesktopWindow> captured_windows;

        // 异步获取可共享内容
        [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* error) {
            if (error || !content) {
                LogError << "Failed to get shareable content: " << error.localizedDescription.UTF8String;
                dispatch_semaphore_signal(semaphore);
                return;
            }

            // 枚举所有窗口
            for (SCWindow* window in content.windows) {
                // 跳过不可见的窗口
                if (!window.isOnScreen) {
                    continue;
                }

                std::wstring class_name;
                if (window.owningApplication) {
                    NSString* bundleId = window.owningApplication.bundleIdentifier ? window.owningApplication.bundleIdentifier : @"";
                    class_name = MAA_NS::to_u16([bundleId UTF8String]);
                }

                NSString* title = window.title ? window.title : @"";
                std::wstring window_name = MAA_NS::to_u16([title UTF8String]);

                // 将CGWindowID转换为void*
                void* hwnd = (void*)(uintptr_t)window.windowID;

                captured_windows.emplace_back(DesktopWindow {
                    .hwnd = hwnd,
                    .class_name = class_name,
                    .window_name = window_name,
                });
            }

            dispatch_semaphore_signal(semaphore);
        }];

        // 等待异步操作完成，超时5秒
        if (dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) != 0) {
            LogError << "Timeout waiting for shareable content";
            return {};
        }

        windows = std::move(captured_windows);
    }
    else {
        LogError << "macOS 12.3 or later required for window enumeration";
    }

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif // __APPLE__
