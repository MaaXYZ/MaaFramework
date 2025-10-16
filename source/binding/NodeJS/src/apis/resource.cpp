#include "loader.h"

#include <thread>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/classes.h"
#include "../foundation/macros.h"
#include "bridge.h"
#include "ext.h"

struct ResourceImpl : public maajs::NativeClassBase
{
    MaaResource* resource;

    ResourceImpl(MaaResource* res)
        : resource(res)
    {
        std::cout << "resource create!" << std::endl;
    }

    ~ResourceImpl() { std::cout << "resource destroy!" << std::endl; }

    void destroy() { MaaResourceDestroy(resource); }

    maajs::ValueType post_bundle(maajs::ConstValueType self, maajs::EnvType env, std::string path)
    {
        auto id = MaaResourcePostBundle(resource, path.c_str());
        return maajs::CallCtorHelper(env, ExtContext::get(env)->jobCtor, maajs::DupValue(env, self), id);
    }

    MaaStatus status(MaaResId id) { return MaaResourceStatus(resource, id); }

    maajs::PromiseType wait(maajs::EnvType env, MaaResId id)
    {
        auto [pro, resolve] = maajs::MakePromise(env);

        // TODO: NodeJS thread safe functions + async work
        std::ignore = id;

#ifdef MAA_JS_IMPL_IS_QUICKJS
        auto bridge = QuickJSRuntimeBridgeInterface::get(env);

        (std::thread {
             [bridge, handle = resource, id = id, resolve]() {
                 auto status = MaaResourceWait(handle, id);
                 bridge->push_task([resolve, status](JSContext* ctx) {
                     maajs::EnvType env {
                         JS_GetRuntime(ctx),
                         ctx,
                     };
                     maajs::CallFuncHelper<void>(env, resolve->Value(), status);
                 });
             },
         })
            .detach();
#endif

        return pro;
    }

    constexpr static char name[] = "Resource";
    constexpr static bool need_gc_mark = false;

    static ResourceImpl* ctor([[maybe_unused]] const maajs::CallbackInfo& info)
    {
        auto handle = MaaResourceCreate(nullptr, nullptr);
        if (!handle) {
            return nullptr;
        }
        return new ResourceImpl { handle };
    }

    static void init_proto([[maybe_unused]] maajs::EnvType env, [[maybe_unused]] maajs::ConstObjectType proto);
};

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ResourceImpl)

void ResourceImpl::init_proto(maajs::EnvType env, [[maybe_unused]] maajs::ConstObjectType proto)
{
    MAA_BIND_FUNC(env, proto, "destroy", ResourceImpl::destroy);
    MAA_BIND_FUNC(env, proto, "post_bundle", ResourceImpl::post_bundle);
    MAA_BIND_FUNC(env, proto, "status", ResourceImpl::status);
    MAA_BIND_FUNC(env, proto, "wait", ResourceImpl::wait);
}

maajs::ValueType load_resource(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::NativeClass<ResourceImpl>::init(env, ctor);
    ExtContext::get(env)->resourceCtor = maajs::PersistentFunction(env, ctor);
    return ctor;
}
