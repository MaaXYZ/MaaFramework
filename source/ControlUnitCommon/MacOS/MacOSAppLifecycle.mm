#include "MacOSAppLifecycle.h"

#import <AppKit/AppKit.h>

#include <libproc.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{
bool process_exists(pid_t pid)
{
    proc_bsdinfo info { };
    return proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) == static_cast<int>(sizeof(info));
}

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

        bool stopped = true;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        std::vector<pid_t> pending;
        for (NSRunningApplication* app in apps) {
            const pid_t pid = app.processIdentifier;
            if (![app terminate]) {
                LogError << "Failed to send quit request" << VAR(pid);
                stopped = false;
                continue;
            }

            pending.push_back(pid);
        }

        while (!pending.empty()) {
            std::erase_if(pending, [](pid_t pid) { return !process_exists(pid); });
            if (pending.empty()) {
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                for (const pid_t pid : pending) {
                    LogError << "Application did not quit after the quit request" << VAR(pid);
                }
                stopped = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return stopped;
    }
}
}

MAA_CTRL_UNIT_NS_END
