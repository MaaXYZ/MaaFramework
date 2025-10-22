#include "controller.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/spec.h"
#include "../utils/library.h"
#include "buffer.h"
#include "ext.h"

static void ControllerSink(void* controller, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto ctrl = ControllerImpl::locate_object(fn.Env(), reinterpret_cast<MaaController*>(controller));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    ctrl,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
}

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
    if (controller) {
        ExtContext::get(env)->controllers.del(controller);
    }

    if (own && controller) {
        for (const auto& [id, ctx] : sinks) {
            MaaControllerRemoveSink(controller, id);
            delete ctx;
        }
        sinks.clear();

        MaaControllerDestroy(controller);
    }
    controller = nullptr;
    own = false;
}

MaaSinkId ControllerImpl::add_sink(maajs::FunctionType sink)
{
    if (!own) {
        return MaaInvalidId;
    }

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
        std::vector<AdbDevice> infos(size);
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
