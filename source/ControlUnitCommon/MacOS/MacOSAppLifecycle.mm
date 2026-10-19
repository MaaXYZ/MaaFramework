#include "MacOSAppLifecycle.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

#include <chrono>
#include <memory>
#include <thread>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{
class EventOwner
{
public:
    explicit EventOwner(CGEventRef event = nullptr)
        : event_(event)
    {
    }
    ~EventOwner()
    {
        if (event_) {
            CFRelease(event_);
        }
    }
    EventOwner(const EventOwner&) = delete;
    EventOwner& operator=(const EventOwner&) = delete;
    CGEventRef get() const { return event_; }

private:
    CGEventRef event_ = nullptr;
};

NSURL* find_application(NSString* target, const std::string& search_path)
{
    if ([target hasPrefix:@"/"]) {
        return [NSURL fileURLWithPath:target];
    }

    NSString* directory = [[NSString stringWithUTF8String:search_path.c_str()] stringByExpandingTildeInPath];
    NSDirectoryEnumerator<NSURL*>* enumerator = [[NSFileManager defaultManager]
                   enumeratorAtURL:[NSURL fileURLWithPath:directory]
        includingPropertiesForKeys:nil
                           options:NSDirectoryEnumerationSkipsHiddenFiles | NSDirectoryEnumerationSkipsPackageDescendants
                      errorHandler:nil];
    for (NSURL* candidate in enumerator) {
        if (![candidate.pathExtension.lowercaseString isEqualToString:@"app"]) {
            continue;
        }
        if ([[NSBundle bundleWithURL:candidate].bundleIdentifier isEqualToString:target]) {
            return candidate;
        }
    }
    return [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:target];
}
}

namespace MacOSAppLifecycle
{
bool start_app(const std::string& intent, const std::string& search_path)
{
    LogFunc << VAR(intent) << VAR(search_path);

    @autoreleasepool {
        NSString* target = [NSString stringWithUTF8String:intent.c_str()];
        if (target.length == 0) {
            LogError << "Invalid application intent" << VAR(intent);
            return false;
        }

        NSURL* url = find_application(target, search_path);
        if (!url || ![NSBundle bundleWithURL:url].bundleIdentifier) {
            LogError << "Application bundle not found" << VAR(intent) << VAR(search_path);
            return false;
        }

        struct LaunchResult
        {
            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
            bool started = false;
            std::string error;

            ~LaunchResult() { dispatch_release(semaphore); }
        };
        // The completion handler may outlive this call on the main thread or after a timeout.
        const auto result = std::make_shared<LaunchResult>();
        [[NSWorkspace sharedWorkspace] openApplicationAtURL:url
                                              configuration:[NSWorkspaceOpenConfiguration configuration]
                                          completionHandler:^(NSRunningApplication* app, NSError* error) {
                                              result->started = app != nil;
                                              if (error) {
                                                  result->error = error.localizedDescription.UTF8String;
                                              }
                                              if (!result->started) {
                                                  LogError << "Application launch failed" << VAR(result->error);
                                              }
                                              dispatch_semaphore_signal(result->semaphore);
                                          }];
        if ([NSThread isMainThread]) {
            LogWarn << "Skipping launch completion wait on the main thread. Success only indicates the launch request was sent."
                    << VAR(intent);
            return true;
        }
        if (dispatch_semaphore_wait(result->semaphore, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC)) != 0) {
            LogError << "Application launch timed out" << VAR(intent);
            return false;
        }
        return result->started;
    }
}

bool stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    @autoreleasepool {
        NSString* bundle_id = [NSString stringWithUTF8String:intent.c_str()];
        if (intent.starts_with("/")) {
            bundle_id = [NSBundle bundleWithPath:bundle_id].bundleIdentifier;
        }
        if (bundle_id.length == 0) {
            LogError << "Invalid application bundle identifier" << VAR(intent);
            return false;
        }
        NSArray<NSRunningApplication*>* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:bundle_id];
        if (apps.count == 0) {
            LogInfo << "Application is not running" << VAR(intent);
            return true;
        }
        if (!AXIsProcessTrusted()) {
            LogError << "Accessibility permission is required to stop application" << VAR(intent);
            return false;
        }

        bool stopped = true;
        for (NSRunningApplication* app in apps) {
            const pid_t pid = app.processIdentifier;
            constexpr CGKeyCode QuitKey = 0x0C; // Q on the macOS virtual keyboard.
            EventOwner key_down(CGEventCreateKeyboardEvent(nullptr, QuitKey, true));
            EventOwner key_up(CGEventCreateKeyboardEvent(nullptr, QuitKey, false));
            if (!key_down.get() || !key_up.get()) {
                LogError << "Failed to create application quit shortcut" << VAR(pid);
                stopped = false;
                continue;
            }
            CGEventSetFlags(key_down.get(), kCGEventFlagMaskCommand);
            CGEventSetFlags(key_up.get(), kCGEventFlagMaskCommand);
            CGEventPostToPid(pid, key_down.get());
            CGEventPostToPid(pid, key_up.get());

            if ([NSThread isMainThread]) {
                LogWarn << "Command-Q sent; skipping exit detection on the main thread. Success only indicates the quit request was sent."
                        << VAR(pid);
                continue;
            }

            for (int attempt = 0; attempt < 20 && !app.isTerminated; ++attempt) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            if (!app.isTerminated) {
                LogError << "Application did not quit after Command-Q" << VAR(pid);
                stopped = false;
            }
        }
        return stopped;
    }
}
}

MAA_CTRL_UNIT_NS_END
