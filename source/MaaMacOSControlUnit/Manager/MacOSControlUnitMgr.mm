#include "MacOSControlUnitMgr.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"

#include "Input/GestureInput.h"
#include "Input/GlobalEventInput.h"
#include "Input/InputUtils.h"
#include "Input/PostToPidInput.h"
#include "Screencap/ScreenCaptureKitScreencap.h"

MAA_CTRL_UNIT_NS_BEGIN

MacOSControlUnitMgr::MacOSControlUnitMgr(uint32_t window_id, MaaMacOSScreencapMethod screencap_method, MaaMacOSInputMethod input_method)
    : window_id_(window_id)
    , screencap_method_(screencap_method)
    , input_method_(input_method)
{
}

bool MacOSControlUnitMgr::connect()
{
    connected_ = false;

    switch (screencap_method_) {
    case MaaMacOSScreencapMethod_ScreenCaptureKit:
        // 检查macOS版本
        if (__builtin_available(macOS 14.0, *)) {
            screencap_ = std::make_shared<ScreenCaptureKitScreencap>(window_id_);
        }
        else {
            LogError << "macOS 14.0 or later required for ScreenCaptureKit";
            return false;
        }
        break;
    case MaaMacOSScreencapMethod_None:
        LogWarn << "No screencap method specified, screencap will not work";
        break;
    default:
        LogError << "Unknown screencap method: " << static_cast<int>(screencap_method_);
        break;
    }

    switch (input_method_) {
    case MaaMacOSInputMethod_GlobalEvent:
        input_ = std::make_shared<GlobalEventInput>(window_id_);
        break;
    case MaaMacOSInputMethod_PostToPid:
        input_ = std::make_shared<PostToPidInput>(window_id_);
        break;
    case MaaMacOSInputMethod_Gesture:
        input_ = std::make_shared<GestureInput>(window_id_);
        break;
    case MaaMacOSInputMethod_None:
        LogWarn << "No input method specified, input will not work";
        break;
    default:
        LogError << "Unknown input method: " << static_cast<int>(input_method_);
        break;
    }

    connected_ = true;
    return true;
}

bool MacOSControlUnitMgr::connected() const
{
    return connected_;
}

bool MacOSControlUnitMgr::request_uuid(std::string& uuid)
{
    uuid = std::to_string(window_id_);
    return true;
}

MaaControllerFeature MacOSControlUnitMgr::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_None;
    if (input_) {
        feat |= input_->get_features()
            & (MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick);
    }
    return feat;
}

bool MacOSControlUnitMgr::start_app(const std::string& intent)
{
    const auto& path = intent;
    NSString* nspath = [NSString stringWithUTF8String:path.c_str()];
    NSBundle* bundle = [NSBundle bundleWithPath:nspath];
    if (!bundle) {
        LogError << "start game " << path << " failed: game bundle id not found!";
        return false;
    }

    struct LaunchResult
    {
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        bool started = false;
        std::string error;

        ~LaunchResult() { dispatch_release(semaphore); }
    };
    const auto result = std::make_shared<LaunchResult>();
    [[NSWorkspace sharedWorkspace] openApplicationAtURL:[NSURL fileURLWithPath:nspath]
                                          configuration:[NSWorkspaceOpenConfiguration configuration]
                                      completionHandler:^(NSRunningApplication* app, NSError* error) {
                                          result->started = app != nil;
                                          if (error) {
                                              result->error = error.localizedDescription.UTF8String;
                                          }
                                          dispatch_semaphore_signal(result->semaphore);
                                      }];
    if (dispatch_semaphore_wait(result->semaphore, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC)) != 0) {
        LogError << "start game " << path << " timed out";
        return false;
    }
    if (!result->started) {
        LogError << "start game " << path << " failed: " << result->error;
        return false;
    }
    return true;
}

// send Command+Q to stop app
bool MacOSControlUnitMgr::stop_app(const std::string& intent)
{
    const auto& path = intent;
    NSString* nspath = [NSString stringWithUTF8String:path.c_str()];
    NSBundle* bundle = [NSBundle bundleWithPath:nspath];
    if (!bundle || !bundle.bundleIdentifier) {
        LogError << "stop game " << path << " failed: game bundle id not found!";
        return false;
    }
    NSString* bundle_id = bundle.bundleIdentifier;
    NSArray<NSRunningApplication*>* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:bundle_id];
    if (apps.count == 0) {
        LogInfo << "stop game: no known running game";
        return true;
    }
    bool stopped = true;
    for (NSRunningApplication* app in apps) {
        const pid_t pid = app.processIdentifier;
        if (!AXIsProcessTrusted()) {
            LogError << "Accessibility permission is required to stop game.";
            stopped = false;
            continue;
        }

        constexpr CGKeyCode QuitKey = 0x0C; // Q on the macOS virtual keyboard.
        EventOwner key_down(CGEventCreateKeyboardEvent(nullptr, QuitKey, true));
        EventOwner key_up(CGEventCreateKeyboardEvent(nullptr, QuitKey, false));
        if (!key_down.get() || !key_up.get()) {
            LogError << "stop game could not create a quit shortcut for the game process: " << pid;
            stopped = false;
            continue;
        }
        CGEventSetFlags(key_down.get(), kCGEventFlagMaskCommand);
        CGEventSetFlags(key_up.get(), kCGEventFlagMaskCommand);
        CGEventPostToPid(pid, key_down.get());
        CGEventPostToPid(pid, key_up.get());

        for (int attempt = 0; attempt < 20 && !app.isTerminated; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        if (!app.isTerminated) {
            LogError << "game did not quit after Command-Q: " << pid;
            stopped = false;
        }
    }
    return stopped;
}

bool MacOSControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is nullptr";
        return false;
    }

    auto opt = screencap_->screencap();
    if (!opt) {
        LogError << "screencap failed";
        return false;
    }

    image = std::move(*opt);
    return true;
}

bool MacOSControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click(x, y);
}

bool MacOSControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool MacOSControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool MacOSControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool MacOSControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_up(contact);
}

bool MacOSControlUnitMgr::relative_move(int dx, int dy)
{
    (void)dx;
    (void)dy;
    LogWarn << "relative_move not supported on macOS controller";
    return false;
}

bool MacOSControlUnitMgr::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click_key(key);
}

bool MacOSControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->input_text(text);
}

bool MacOSControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_down(key);
}

bool MacOSControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_up(key);
}

bool MacOSControlUnitMgr::scroll(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->scroll(dx, dy);
}

bool MacOSControlUnitMgr::inactive()
{
    return true;
}

json::object MacOSControlUnitMgr::get_info() const
{
    json::object info;
    info["type"] = "macos";
    info["window_id"] = window_id_;
    info["screencap_method"] = static_cast<int64_t>(screencap_method_);
    info["input_method"] = static_cast<int64_t>(input_method_);
    return info;
}

MAA_CTRL_UNIT_NS_END
