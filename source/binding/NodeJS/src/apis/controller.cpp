#include "controller.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/async.h"
#include "../foundation/classes.h"
#include "../foundation/macros.h"
#include "buffer.h"
#include "ext.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ImageJobImpl)

maajs::ValueType ImageJobImpl::get(maajs::EnvType)
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

void ImageJobImpl::init_proto(maajs::EnvType env, maajs::ObjectType proto, maajs::FunctionType)
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

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ControllerImpl)

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
    if (own) {
        MaaControllerDestroy(controller);
    }
}

maajs::ValueType ControllerImpl::post_connection(maajs::ValueType self, maajs::EnvType env)
{
    auto id = MaaControllerPostConnection(controller);
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ControllerImpl::post_screencap(maajs::ValueType self, maajs::EnvType env)
{
    auto id = MaaControllerPostScreencap(controller);
    return maajs::CallCtorHelper(ExtContext::get(env)->imageJobCtor, self, id);
}

MaaStatus ControllerImpl::status(MaaCtrlId id)
{
    return MaaControllerStatus(controller, id);
}

maajs::PromiseType ControllerImpl::wait(maajs::EnvType env, MaaCtrlId id)
{
    auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = controller, id]() { return MaaControllerWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool ControllerImpl::get_connected()
{
    return MaaControllerConnected(controller);
}

std::optional<maajs::ArrayBufferType> ControllerImpl::get_cached_image(maajs::EnvType env)
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

ControllerImpl* ControllerImpl::ctor(const maajs::CallbackInfo&)
{
    return nullptr;
}

void ControllerImpl::init_proto(maajs::EnvType env, maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", ControllerImpl::destroy);
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

MAA_JS_NATIVE_CLASS_STATIC_IMPL(AdbControllerImpl)

std::string AdbControllerImpl::agent_path()
{
    return "not impl yet";
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

void AdbControllerImpl::init_proto(maajs::EnvType env, maajs::ObjectType, maajs::FunctionType ctor)
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
