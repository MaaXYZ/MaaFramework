#include "controller.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/spec.h"
#include "../utils/library.h"
#include "buffer.h"
#include "callback.h"
#include "ext.h"

maajs::ValueType ImageJobImpl::get()
{
    return source.Value()["cached_image"];
}

ImageJobImpl* ImageJobImpl::ctor(const maajs::CallbackInfo& info)
{
    auto job = new ImageJobImpl;
    job->source = maajs::PersistentObject(info[0].As<maajs::ObjectType>());
    job->id = maajs::JSConvert<uint64_t>::from_value(info[1]);
    return job;
}

void ImageJobImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "get", ImageJobImpl::get);
}

maajs::ValueType load_image_job(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ImageJobImpl>::init<JobImpl>(env, ctor, &ExtContext::get(env)->jobCtor);
    ExtContext::get(env)->imageJobCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

ControllerImpl::ControllerImpl(MaaController* ctrl, bool own)
    : controller(ctrl)
    , own(own)
{
}

ControllerImpl::~ControllerImpl()
{
    destroy();
}

void ControllerImpl::destroy()
{
    if (!controller) {
        return;
    }

    ExtContext::get(env)->controllers.del(controller);

    for (const auto& [id, ctx] : sinks) {
        MaaControllerRemoveSink(controller, id);
    }
    sinks.clear();

    if (own) {
        MaaControllerDestroy(controller);
    }

    controller = nullptr;
    own = false;
}

MaaSinkId ControllerImpl::add_sink(maajs::FunctionType sink)
{
    auto ctx = new maajs::CallbackContext(sink, "ControllerSink");
    auto id = MaaControllerAddSink(controller, ControllerSink, ctx);
    if (id != MaaInvalidId) {
        sinks[id] = ctx;
    }
    else {
        delete ctx;
    }
    return id;
}

void ControllerImpl::remove_sink(MaaSinkId id)
{
    if (auto it = sinks.find(id); it != sinks.end()) {
        MaaControllerRemoveSink(controller, id);
        delete it->second;
        sinks.erase(it);
    }
}

void ControllerImpl::clear_sinks()
{
    MaaControllerClearSinks(controller);
    for (const auto& [_, ctx] : sinks) {
        delete ctx;
    }
    sinks.clear();
}

void ControllerImpl::set_screenshot_target_long_side(int32_t value)
{
    if (!MaaControllerSetOption(controller, MaaCtrlOption_ScreenshotTargetLongSide, &value, sizeof(value))) {
        throw maajs::MaaError { "Controller set screenshot_target_long_side failed" };
    }
}

void ControllerImpl::set_screenshot_target_short_side(int32_t value)
{
    if (!MaaControllerSetOption(controller, MaaCtrlOption_ScreenshotTargetShortSide, &value, sizeof(value))) {
        throw maajs::MaaError { "Controller set screenshot_target_short_side failed" };
    }
}

void ControllerImpl::set_screenshot_use_raw_size(bool value)
{
    if (!MaaControllerSetOption(controller, MaaCtrlOption_ScreenshotUseRawSize, &value, sizeof(value))) {
        throw maajs::MaaError { "Controller set screenshot_use_raw_size failed" };
    }
}

maajs::ValueType ControllerImpl::post_connection(maajs::ValueType self, maajs::EnvType)
{
    auto id = MaaControllerPostConnection(controller);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_click(maajs::ValueType self, maajs::EnvType, int32_t x, int32_t y)
{
    auto id = MaaControllerPostClick(controller, x, y);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType
    ControllerImpl::post_swipe(maajs::ValueType self, maajs::EnvType, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
{
    auto id = MaaControllerPostSwipe(controller, x1, y1, x2, y2, duration);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_click_key(maajs::ValueType self, maajs::EnvType, int32_t keycode)
{
    auto id = MaaControllerPostClickKey(controller, keycode);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_input_text(maajs::ValueType self, maajs::EnvType, std::string text)
{
    auto id = MaaControllerPostInputText(controller, text.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_start_app(maajs::ValueType self, maajs::EnvType, std::string intent)
{
    auto id = MaaControllerPostStartApp(controller, intent.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_stop_app(maajs::ValueType self, maajs::EnvType, std::string intent)
{
    auto id = MaaControllerPostStopApp(controller, intent.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType
    ControllerImpl::post_touch_down(maajs::ValueType self, maajs::EnvType, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    auto id = MaaControllerPostTouchDown(controller, contact, x, y, pressure);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType
    ControllerImpl::post_touch_move(maajs::ValueType self, maajs::EnvType, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    auto id = MaaControllerPostTouchMove(controller, contact, x, y, pressure);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_touch_up(maajs::ValueType self, maajs::EnvType, int32_t contact)
{
    auto id = MaaControllerPostTouchUp(controller, contact);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_key_down(maajs::ValueType self, maajs::EnvType, int32_t keycode)
{
    auto id = MaaControllerPostKeyDown(controller, keycode);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_key_up(maajs::ValueType self, maajs::EnvType, int32_t keycode)
{
    auto id = MaaControllerPostKeyUp(controller, keycode);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_scroll(maajs::ValueType self, maajs::EnvType, int32_t dx, int32_t dy)
{
    auto id = MaaControllerPostScroll(controller, dx, dy);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_screencap(maajs::ValueType self, maajs::EnvType)
{
    auto id = MaaControllerPostScreencap(controller);
    return maajs::CallCtorHelper(ExtContext::get(env)->imageJobCtor, self, id);
}

MaaStatus ControllerImpl::status(MaaCtrlId id)
{
    return MaaControllerStatus(controller, id);
}

maajs::PromiseType ControllerImpl::wait(MaaCtrlId id)
{
    auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = controller, id]() { return MaaControllerWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool ControllerImpl::get_connected()
{
    return MaaControllerConnected(controller);
}

std::optional<maajs::ArrayBufferType> ControllerImpl::get_cached_image()
{
    ImageBuffer buf;
    if (!MaaControllerCachedImage(controller, buf)) {
        return std::nullopt;
    }
    return buf.data(env);
}

std::optional<std::string> ControllerImpl::get_uuid()
{
    StringBuffer buf;
    if (!MaaControllerGetUuid(controller, buf)) {
        return std::nullopt;
    }
    return buf.str();
}

std::string ControllerImpl::to_string()
{
    return std::format(" handle = {:#018x}, {} ", reinterpret_cast<uintptr_t>(controller), own ? "owned" : "rented");
}

maajs::ValueType ControllerImpl::locate_object(maajs::EnvType env, MaaController* ctrl)
{
    if (auto obj = ExtContext::get(env)->controllers.find(ctrl)) {
        return *obj;
    }
    else {
        return maajs::CallCtorHelper(ExtContext::get(env)->controllerCtor, std::to_string(reinterpret_cast<uintptr_t>(ctrl)));
    }
}

void ControllerImpl::init_bind(maajs::ObjectType self)
{
    ExtContext::get(env)->controllers.add(controller, self);
}

void ControllerImpl::gc_mark(maajs::NativeMarkerFunc marker)
{
    for (const auto& [_, ctx] : sinks) {
        marker(ctx->fn);
    }
}

ControllerImpl* ControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    if (info.Length() == 1) {
        try {
            MaaController* handle = reinterpret_cast<MaaController*>(std::stoull(info[0].As<maajs::StringType>().Utf8Value()));
            return new ControllerImpl { handle, false };
        }
        catch (std::exception&) {
            return nullptr;
        }
    }
    return nullptr;
}

void ControllerImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", ControllerImpl::destroy);
    MAA_BIND_FUNC(proto, "add_sink", ControllerImpl::add_sink);
    MAA_BIND_FUNC(proto, "remove_sink", ControllerImpl::remove_sink);
    MAA_BIND_SETTER(proto, "screenshot_target_long_side", ControllerImpl::set_screenshot_target_long_side);
    MAA_BIND_SETTER(proto, "screenshot_target_short_side", ControllerImpl::set_screenshot_target_short_side);
    MAA_BIND_SETTER(proto, "screenshot_use_raw_size", ControllerImpl::set_screenshot_use_raw_size);
    MAA_BIND_FUNC(proto, "clear_sinks", ControllerImpl::clear_sinks);
    MAA_BIND_FUNC(proto, "post_connection", ControllerImpl::post_connection);
    MAA_BIND_FUNC(proto, "post_click", ControllerImpl::post_click);
    MAA_BIND_FUNC(proto, "post_swipe", ControllerImpl::post_swipe);
    MAA_BIND_FUNC(proto, "post_click_key", ControllerImpl::post_click_key);
    MAA_BIND_FUNC(proto, "post_input_text", ControllerImpl::post_input_text);
    MAA_BIND_FUNC(proto, "post_start_app", ControllerImpl::post_start_app);
    MAA_BIND_FUNC(proto, "post_stop_app", ControllerImpl::post_stop_app);
    MAA_BIND_FUNC(proto, "post_touch_down", ControllerImpl::post_touch_down);
    MAA_BIND_FUNC(proto, "post_touch_move", ControllerImpl::post_touch_move);
    MAA_BIND_FUNC(proto, "post_touch_up", ControllerImpl::post_touch_up);
    MAA_BIND_FUNC(proto, "post_key_down", ControllerImpl::post_key_down);
    MAA_BIND_FUNC(proto, "post_key_up", ControllerImpl::post_key_up);
    MAA_BIND_FUNC(proto, "post_scroll", ControllerImpl::post_scroll);
    MAA_BIND_FUNC(proto, "post_screencap", ControllerImpl::post_screencap);
    MAA_BIND_FUNC(proto, "status", ControllerImpl::status);
    MAA_BIND_FUNC(proto, "wait", ControllerImpl::wait);
    MAA_BIND_GETTER(proto, "connected", ControllerImpl::get_connected);
    MAA_BIND_GETTER(proto, "cached_image", ControllerImpl::get_cached_image);
    MAA_BIND_GETTER(proto, "uuid", ControllerImpl::get_uuid);
}

maajs::ValueType load_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ControllerImpl>::init(env, ctor);
    ExtContext::get(env)->controllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

std::string AdbControllerImpl::agent_path()
{
    // node_modules/@maaxyz/maa-node-{os}-{cpu}
    auto folder = get_library_path(reinterpret_cast<void*>(agent_path)).parent_path();
    // node_modules/@maaxyz/maa-node/agent
    return folder.parent_path().append("maa-node").append("agent").string();
}

maajs::PromiseType AdbControllerImpl::find(maajs::EnvType env, maajs::OptionalParam<std::string> adb)
{
    using Result = std::optional<std::vector<AdbDevice>>;
    auto worker = new maajs::AsyncWork<Result>(env, [adb]() -> Result {
        auto lst = MaaToolkitAdbDeviceListCreate();
        bool succ = false;
        if (adb) {
            succ = MaaToolkitAdbDeviceFindSpecified(adb->c_str(), lst);
        }
        else {
            succ = MaaToolkitAdbDeviceFind(lst);
        }
        if (!succ) {
            MaaToolkitAdbDeviceListDestroy(lst);
            return std::nullopt;
        }

        std::vector<AdbDevice> result;
        auto size = MaaToolkitAdbDeviceListSize(lst);
        result.reserve(size);
        for (size_t i = 0; i < size; i++) {
            auto dev = MaaToolkitAdbDeviceListAt(lst, i);
            result.push_back(
                std::make_tuple(
                    std::string(MaaToolkitAdbDeviceGetName(dev)),
                    std::string(MaaToolkitAdbDeviceGetAdbPath(dev)),
                    std::string(MaaToolkitAdbDeviceGetAddress(dev)),
                    MaaToolkitAdbDeviceGetScreencapMethods(dev),
                    MaaToolkitAdbDeviceGetInputMethods(dev),
                    std::string(MaaToolkitAdbDeviceGetConfig(dev))));
        }
        MaaToolkitAdbDeviceListDestroy(lst);

        return result;
    });
    worker->Queue();
    return worker->Promise();
}

AdbControllerImpl* AdbControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    auto [adb_path, address, screencap_methods, input_methods, config, agent] = maajs::UnWrapArgs<AdbControllerCtorParam, void>(info);
    auto ctrl = MaaAdbControllerCreate(
        adb_path.c_str(),
        address.c_str(),
        screencap_methods,
        input_methods,
        config.c_str(),
        agent.value_or(agent_path()).c_str());
    if (!ctrl) {
        return nullptr;
    }
    return new AdbControllerImpl(ctrl, true);
}

void AdbControllerImpl::init_proto(maajs::ObjectType, maajs::FunctionType ctor)
{
    MAA_BIND_FUNC(ctor, "agent_path", agent_path);
    MAA_BIND_FUNC(ctor, "find", find);
}

maajs::ValueType load_adb_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<AdbControllerImpl>::init<ControllerImpl>(env, ctor, &ExtContext::get(env)->controllerCtor);
    ExtContext::get(env)->adbControllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

maajs::PromiseType Win32ControllerImpl::find(maajs::EnvType env)
{
    using Result = std::optional<std::vector<Win32Device>>;
    auto worker = new maajs::AsyncWork<Result>(env, []() -> Result {
        auto lst = MaaToolkitDesktopWindowListCreate();
        if (!MaaToolkitDesktopWindowFindAll(lst)) {
            MaaToolkitDesktopWindowListDestroy(lst);
            return std::nullopt;
        }

        std::vector<Win32Device> result;
        auto size = MaaToolkitDesktopWindowListSize(lst);
        result.reserve(size);
        for (size_t i = 0; i < size; i++) {
            auto dev = MaaToolkitDesktopWindowListAt(lst, i);
            result.push_back(
                std::make_tuple(
                    reinterpret_cast<uintptr_t>(MaaToolkitDesktopWindowGetHandle(dev)),
                    std::string(MaaToolkitDesktopWindowGetClassName(dev)),
                    std::string(MaaToolkitDesktopWindowGetWindowName(dev))));
        }
        MaaToolkitDesktopWindowListDestroy(lst);

        return result;
    });
    worker->Queue();
    return worker->Promise();
}

Win32ControllerImpl* Win32ControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    auto [hwnd, screencap_method, mouse_method, keyboard_method] = maajs::UnWrapArgs<Win32ControllerCtorParam, void>(info);
    auto ctrl = MaaWin32ControllerCreate(reinterpret_cast<void*>(hwnd), screencap_method, mouse_method, keyboard_method);
    if (!ctrl) {
        return nullptr;
    }
    return new Win32ControllerImpl(ctrl, true);
}

void Win32ControllerImpl::init_proto(maajs::ObjectType, maajs::FunctionType ctor)
{
    MAA_BIND_FUNC(ctor, "find", find);
}

maajs::ValueType load_win32_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<Win32ControllerImpl>::init<ControllerImpl>(env, ctor, &ExtContext::get(env)->controllerCtor);
    ExtContext::get(env)->win32ControllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

#ifdef __APPLE__
PlayCoverControllerImpl* PlayCoverControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    auto [address, uuid] = maajs::UnWrapArgs<PlayCoverControllerCtorParam, void>(info);
    auto ctrl = MaaPlayCoverControllerCreate(address.c_str(), uuid.c_str());
    if (!ctrl) {
        return nullptr;
    }
    return new PlayCoverControllerImpl(ctrl, true);
}

void PlayCoverControllerImpl::init_proto(maajs::ObjectType, maajs::FunctionType)
{
}

maajs::ValueType load_playcover_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<PlayCoverControllerImpl>::init<ControllerImpl>(env, ctor, &ExtContext::get(env)->controllerCtor);
    ExtContext::get(env)->playcoverControllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
#endif

DbgControllerImpl* DbgControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    auto [read_path, write_path, type, config] = maajs::UnWrapArgs<DbgControllerCtorParam, void>(info);
    auto ctrl = MaaDbgControllerCreate(read_path.c_str(), write_path.c_str(), type, config.c_str());
    if (!ctrl) {
        return nullptr;
    }
    return new DbgControllerImpl(ctrl, true);
}

void DbgControllerImpl::init_proto(maajs::ObjectType, maajs::FunctionType)
{
}

maajs::ValueType load_dbg_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<DbgControllerImpl>::init<ControllerImpl>(env, ctor, &ExtContext::get(env)->controllerCtor);
    ExtContext::get(env)->dbgControllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

CustomControllerContext::~CustomControllerContext()
{
    for (const auto& [_, ctx] : callbacks) {
        delete ctx;
    }
}

void CustomControllerContext::add_bind(
    maajs::EnvType env,
    std::string name,
    std::string func_name,
    int argc,
    std::shared_ptr<maajs::ObjectRefType> actor,
    std::function<maajs::ValueType(maajs::EnvType)> fallback)
{
    callbacks[name] = new maajs::CallbackContext(
        maajs::MakeFunction(
            env,
            func_name.c_str(),
            argc,
            [actor, name, fallback](const auto& info) -> maajs::ValueType {
                auto func = actor->Value()[name].AsValue();
                if (func.IsFunction()) {
                    std::vector<maajs::ValueType> args;
                    args.reserve(info.Length());
                    for (size_t i = 0; i < info.Length(); i++) {
                        args.push_back(info[i]);
                    }
                    return func.As<maajs::FunctionType>().Call(actor->Value(), args);
                }
                else {
                    return fallback(info.Env());
                }
            },
            actor),
        func_name.c_str());
}

void CustomControllerContext::gc_mark(maajs::NativeMarkerFunc marker)
{
    for (const auto& [_, ctx] : callbacks) {
        marker(ctx->fn);
    }
}

void CustomControllerImpl::gc_mark(maajs::NativeMarkerFunc marker)
{
    ControllerImpl::gc_mark(marker);

    if (actor) {
        marker(actor->Value());
    }
    if (context) {
        context->gc_mark(marker);
    }
}

CustomControllerImpl* CustomControllerImpl::ctor(const maajs::CallbackInfo& info)
{
    if (info.Length() != 1) {
        return nullptr;
    }

    auto actor = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(info[0].As<maajs::ObjectType>()));
    auto context = std::make_unique<CustomControllerContext>();
    auto ctrl = MaaCustomControllerCreate(&CustomCallbacks, context.get());
    if (!ctrl) {
        return nullptr;
    }

    auto ret_false = [](maajs::EnvType env2) {
        return maajs::BooleanType::New(env2, false);
    };
    auto ret_null = [](maajs::EnvType env2) {
        return env2.Null();
    };

    context->add_bind(info.Env(), "connect", "CustomConnect", 0, actor, ret_false);
    context->add_bind(info.Env(), "request_uuid", "CustomRequestUuid", 0, actor, ret_null);
    context->add_bind(info.Env(), "get_features", "CustomGetFeatures", 0, actor, ret_null);
    context->add_bind(info.Env(), "start_app", "CustomStartApp", 1, actor, ret_false);
    context->add_bind(info.Env(), "stop_app", "CustomStopApp", 1, actor, ret_false);
    context->add_bind(info.Env(), "screencap", "CustomScreencap", 0, actor, ret_null);
    context->add_bind(info.Env(), "click", "CustomClick", 2, actor, ret_false);
    context->add_bind(info.Env(), "swipe", "CustomSwipe", 5, actor, ret_false);
    context->add_bind(info.Env(), "touch_down", "CustomTouchDown", 4, actor, ret_false);
    context->add_bind(info.Env(), "touch_move", "CustomTouchMove", 4, actor, ret_false);
    context->add_bind(info.Env(), "touch_up", "CustomTouchUp", 1, actor, ret_false);
    context->add_bind(info.Env(), "click_key", "CustomClickKey", 1, actor, ret_false);
    context->add_bind(info.Env(), "input_text", "CustomInputText", 1, actor, ret_false);
    context->add_bind(info.Env(), "key_down", "CustomKeyDown", 1, actor, ret_false);
    context->add_bind(info.Env(), "key_up", "CustomKeyUp", 1, actor, ret_false);
    context->add_bind(info.Env(), "scroll", "CustomScroll", 2, actor, ret_false);

    auto impl = new CustomControllerImpl(ctrl, true);
    impl->actor = actor;
    impl->context = std::move(context);
    return impl;
}

void CustomControllerImpl::init_proto(maajs::ObjectType, maajs::FunctionType)
{
}

maajs::ValueType load_custom_controller(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<CustomControllerImpl>::init<ControllerImpl>(env, ctor, &ExtContext::get(env)->controllerCtor);
    ExtContext::get(env)->win32ControllerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
