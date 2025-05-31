#include "../include/forward.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

#include "../include/cb.h"
#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

std::optional<Napi::External<ControllerInfo>> adb_controller_create(
    Napi::Env env,
    std::string adb_path,
    std::string address,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    std::string config,
    std::string agent_path,
    std::optional<Napi::Function> callback)
{
    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaController* handle = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaAdbControllerCreate(
        adb_path.c_str(),
        address.c_str(),
        screencap_methods,
        input_methods,
        config.c_str(),
        agent_path.c_str(),
        cb,
        ctx);

    if (handle) {
        return Napi::External<ControllerInfo>::New(env, new ControllerInfo { { handle }, ctx }, &DeleteFinalizer<ControllerInfo*>);
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

std::optional<Napi::External<ControllerInfo>> win32_controller_create(
    Napi::Env env,
    __DesktopHandle* hwnd,
    MaaWin32ScreencapMethod screencap_methods,
    MaaWin32InputMethod input_methods,
    std::optional<Napi::Function> callback)
{
    void* h = static_cast<void*>(hwnd);
#if defined(_WIN32)
    if (h && !IsWindow(static_cast<HWND>(h))) {
        return std::nullopt;
    }
#endif
    std::ignore = h;

    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaController* handle = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaWin32ControllerCreate(hwnd, screencap_methods, input_methods, cb, ctx);

    if (handle) {
        return Napi::External<ControllerInfo>::New(env, new ControllerInfo { { handle }, ctx }, &DeleteFinalizer<ControllerInfo*>);
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

std::optional<Napi::External<ControllerInfo>>
    custom_controller_create(Napi::Env env, std::optional<Napi::Function> ctrl, std::optional<Napi::Function> callback)
{
    static MaaCustomControllerCallbacks custom_controller_api = {
        CustomControllerConnect,   CustomControllerRequestUUID, CustomControllerStartApp, CustomControllerStopApp,
        CustomControllerScreencap, CustomControllerClick,       CustomControllerSwipe,    CustomControllerTouchDown,
        CustomControllerTouchMove, CustomControllerTouchUp,     CustomControllerPressKey, CustomControllerInputText
    };

    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaController* handle = nullptr;

    CallbackContext* custom_ctx = nullptr;

    custom_ctx = new CallbackContext { env, ctrl.value(), "CustomControllerCallback" };

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaCustomControllerCreate(&custom_controller_api, custom_ctx, cb, ctx);

    if (handle) {
        return Napi::External<ControllerInfo>::New(
            env,
            new ControllerInfo { { handle }, ctx, custom_ctx },
            &DeleteFinalizer<ControllerInfo*>);
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

std::optional<Napi::External<ControllerInfo>> dbg_controller_create(
    Napi::Env env,
    std::string read_path,
    std::string write_path,
    MaaDbgControllerType type,
    std::string config,
    std::optional<Napi::Function> callback)
{
    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaController* handle = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaDbgControllerCreate(read_path.c_str(), write_path.c_str(), type, config.c_str(), cb, ctx);

    if (handle) {
        return Napi::External<ControllerInfo>::New(env, new ControllerInfo { { handle }, ctx }, &DeleteFinalizer<ControllerInfo*>);
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

void controller_destroy(Napi::External<ControllerInfo> info)
{
    info.Data()->dispose();
}

bool controller_set_option_screenshot_target_long_side(Napi::External<ControllerInfo> info, int32_t size)
{
    return MaaControllerSetOption(info.Data()->handle, MaaCtrlOptionEnum::MaaCtrlOption_ScreenshotTargetLongSide, &size, sizeof(size));
}

bool controller_set_option_screenshot_target_short_side(Napi::External<ControllerInfo> info, int32_t size)
{
    return MaaControllerSetOption(info.Data()->handle, MaaCtrlOptionEnum::MaaCtrlOption_ScreenshotTargetShortSide, &size, sizeof(size));
}

bool controller_set_option_screenshot_use_raw_size(Napi::External<ControllerInfo> info, bool flag)
{
    return MaaControllerSetOption(info.Data()->handle, MaaCtrlOptionEnum::MaaCtrlOption_ScreenshotUseRawSize, &flag, sizeof(flag));
}

bool controller_set_option_recording(Napi::External<ControllerInfo> info, bool flag)
{
    return MaaControllerSetOption(info.Data()->handle, MaaCtrlOptionEnum::MaaCtrlOption_Recording, &flag, sizeof(flag));
}

MaaCtrlId controller_post_connection(Napi::External<ControllerInfo> info)
{
    return MaaControllerPostConnection(info.Data()->handle);
}

MaaCtrlId controller_post_click(Napi::External<ControllerInfo> info, int32_t x, int32_t y)
{
    return MaaControllerPostClick(info.Data()->handle, x, y);
}

MaaCtrlId controller_post_swipe(Napi::External<ControllerInfo> info, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
{
    return MaaControllerPostSwipe(info.Data()->handle, x1, y1, x2, y2, duration);
}

MaaCtrlId controller_post_press_key(Napi::External<ControllerInfo> info, int32_t keycode)
{
    return MaaControllerPostPressKey(info.Data()->handle, keycode);
}

MaaCtrlId controller_post_input_text(Napi::External<ControllerInfo> info, std::string text)
{
    return MaaControllerPostInputText(info.Data()->handle, text.c_str());
}

MaaCtrlId controller_post_start_app(Napi::External<ControllerInfo> info, std::string intent)
{
    return MaaControllerPostStartApp(info.Data()->handle, intent.c_str());
}

MaaCtrlId controller_post_stop_app(Napi::External<ControllerInfo> info, std::string intent)
{
    return MaaControllerPostStopApp(info.Data()->handle, intent.c_str());
}

MaaCtrlId controller_post_touch_down(Napi::External<ControllerInfo> info, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    return MaaControllerPostTouchDown(info.Data()->handle, contact, x, y, pressure);
}

MaaCtrlId controller_post_touch_move(Napi::External<ControllerInfo> info, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    return MaaControllerPostTouchMove(info.Data()->handle, contact, x, y, pressure);
}

MaaCtrlId controller_post_touch_up(Napi::External<ControllerInfo> info, int32_t contact)
{
    return MaaControllerPostTouchUp(info.Data()->handle, contact);
}

MaaCtrlId controller_post_screencap(Napi::External<ControllerInfo> info)
{
    return MaaControllerPostScreencap(info.Data()->handle);
}

MaaStatus controller_status(Napi::External<ControllerInfo> info, MaaCtrlId id)
{
    return MaaControllerStatus(info.Data()->handle, id);
}

Napi::Promise controller_wait(Napi::Env env, Napi::External<ControllerInfo> info, MaaCtrlId id)
{
    auto handle = info.Data()->handle;
    auto worker = new SimpleAsyncWork<MaaStatus, "controller_wait">(env, [handle, id]() { return MaaControllerWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool controller_connected(Napi::External<ControllerInfo> info)
{
    return MaaControllerConnected(info.Data()->handle);
}

std::optional<Napi::ArrayBuffer> controller_cached_image(Napi::Env env, Napi::External<ControllerInfo> info)
{
    ImageBuffer buffer;
    auto ret = MaaControllerCachedImage(info.Data()->handle, buffer);
    if (ret) {
        return buffer.data(env);
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::string> controller_get_uuid(Napi::External<ControllerInfo> info)
{
    StringBuffer buffer;
    auto ret = MaaControllerGetUuid(info.Data()->handle, buffer);
    if (ret) {
        return Napi::String::New(info.Env(), buffer.str());
    }
    else {
        return std::nullopt;
    }
}

void load_instance_controller(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(adb_controller_create);
    BIND(win32_controller_create);
    BIND(custom_controller_create);
    BIND(dbg_controller_create);
    BIND(controller_destroy);
    BIND(controller_set_option_screenshot_target_long_side);
    BIND(controller_set_option_screenshot_target_short_side);
    BIND(controller_set_option_screenshot_use_raw_size);
    BIND(controller_set_option_recording);
    BIND(controller_post_connection);
    BIND(controller_post_click);
    BIND(controller_post_swipe);
    BIND(controller_post_press_key);
    BIND(controller_post_input_text);
    BIND(controller_post_start_app);
    BIND(controller_post_stop_app);
    BIND(controller_post_touch_down);
    BIND(controller_post_touch_move);
    BIND(controller_post_touch_up);
    BIND(controller_post_screencap);
    BIND(controller_status);
    BIND(controller_wait);
    BIND(controller_connected);
    BIND(controller_cached_image);
    BIND(controller_get_uuid);
}
