#include "controller.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

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

void ImageJobImpl::init_proto(maajs::EnvType env, maajs::ObjectType proto)
{
    MAA_BIND_FUNC(proto, "get", ImageJobImpl::get);
}

maajs::ValueType load_image_job(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ImageJobImpl>::init<JobImpl>(env, ctor);
    ExtContext::get(env)->imageJobCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

struct ControllerImpl : public maajs::NativeClassBase
{
    MaaController* controller {};
    bool own = false;

    ControllerImpl() = default;

    ControllerImpl(MaaController* ctrl, bool own)
        : controller(ctrl)
        , own(own)
    {
    }

    void destroy()
    {
        if (own) {
            MaaControllerDestroy(controller);
        }
    }

    maajs::ValueType post_connection(maajs::ValueType self, maajs::EnvType env)
    {
        auto id = MaaControllerPostConnection(controller);
        return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
    }

    maajs::ValueType post_screencap(maajs::ValueType self, maajs::EnvType env)
    {
        auto id = MaaControllerPostScreencap(controller);
        return maajs::CallCtorHelper(ExtContext::get(env)->imageJobCtor, self, id);
    }

    MaaStatus status(MaaCtrlId id) { return MaaControllerStatus(controller, id); }

    maajs::PromiseType wait(maajs::EnvType env, MaaCtrlId id)
    {
        auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = controller, id]() { return MaaControllerWait(handle, id); });
        worker->Queue();
        return worker->Promise();
    }

    bool get_connected() { return MaaControllerConnected(controller); }

    std::optional<maajs::ArrayBufferType> get_cached_image(maajs::EnvType env)
    {
        ImageBuffer buf;
        if (!MaaControllerCachedImage(controller, buf)) {
            return std::nullopt;
        }
        return buf.data(env);
    }

    std::optional<std::string> get_uuid()
    {
        StringBuffer buf;
        if (!MaaControllerGetUuid(controller, buf)) {
            return std::nullopt;
        }
        return buf.str();
    }

    constexpr static char name[] = "Controller";

    static ControllerImpl* ctor(const maajs::CallbackInfo& info)
    {
        try {
            MaaController* handle = reinterpret_cast<MaaController*>(std::stoull(info[0].As<maajs::StringType>().Utf8Value()));
            return new ControllerImpl { handle, false };
        }
        catch (std::exception&) {
            return nullptr;
        }
    }

    static void init_proto([[maybe_unused]] maajs::EnvType env, [[maybe_unused]] maajs::ObjectType proto);
};

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ControllerImpl)

void ControllerImpl::init_proto(maajs::EnvType env, maajs::ObjectType proto)
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
