#include "loader.h"

#include <thread>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/async.h"
#include "../foundation/bridge.h"
#include "../foundation/classes.h"
#include "../foundation/macros.h"
#include "../foundation/utils.h"
#include "buffer.h"
#include "ext.h"

struct ResourceImpl : public maajs::NativeClassBase
{
    MaaResource* resource;

    ResourceImpl(MaaResource* res)
        : resource(res)
    {
    }

    ~ResourceImpl() {}

    void destroy() { MaaResourceDestroy(resource); }

    maajs::ValueType post_bundle(maajs::ValueType self, maajs::EnvType env, std::string path)
    {
        auto id = MaaResourcePostBundle(resource, path.c_str());
        return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
    }

    void override_pipeline(maajs::EnvType env, maajs::ValueType pipeline)
    {
        auto str = maajs::JsonStringify(env, pipeline);

        if (!MaaResourceOverridePipeline(resource, str.c_str())) {
            throw maajs::MaaError { "Resource override_pipeline failed" };
        }
    }

    void override_next(std::string node_name, std::vector<std::string> next_list)
    {
        StringListBuffer buffer;
        buffer.set_vector(next_list, [](auto str) {
            StringBuffer buf;
            buf.set(str);
            return buf;
        });
        if (!MaaResourceOverrideNext(resource, node_name.c_str(), buffer)) {
            throw maajs::MaaError { "Resource override_next failed" };
        }
    }

    std::optional<std::string> get_node_data(std::string node_name)
    {
        StringBuffer buffer;
        if (!MaaResourceGetNodeData(resource, node_name.c_str(), buffer)) {
            return std::nullopt;
        }
        return buffer.str();
    }

    void clear() { MaaResourceClear(resource); }

    MaaStatus status(MaaResId id) { return MaaResourceStatus(resource, id); }

    maajs::PromiseType wait(maajs::EnvType env, MaaResId id)
    {
        auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = resource, id]() { return MaaResourceWait(handle, id); });
        worker->Queue();
        return worker->Promise();
    }

    bool get_loaded() { return MaaResourceLoaded(resource); }

    std::optional<std::string> get_hash()
    {
        StringBuffer buf;
        if (!MaaResourceGetHash(resource, buf)) {
            return std::nullopt;
        }
        return buf.str();
    }

    std::optional<std::vector<std::string>> get_node_list()
    {
        StringListBuffer buffer;
        if (!MaaResourceGetNodeList(resource, buffer)) {
            return std::nullopt;
        }

        return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
    }

    constexpr static char name[] = "Resource";
    constexpr static bool need_gc_mark = false;

    static ResourceImpl* ctor([[maybe_unused]] const maajs::CallbackInfo& info)
    {
        auto handle = MaaResourceCreate();
        if (!handle) {
            return nullptr;
        }
        return new ResourceImpl { handle };
    }

    static void init_proto([[maybe_unused]] maajs::EnvType env, [[maybe_unused]] maajs::ObjectType proto);
};

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ResourceImpl)

void ResourceImpl::init_proto(maajs::EnvType env, [[maybe_unused]] maajs::ObjectType proto)
{
    MAA_BIND_FUNC(proto, "destroy", ResourceImpl::destroy);
    MAA_BIND_FUNC(proto, "post_bundle", ResourceImpl::post_bundle);
    MAA_BIND_FUNC(proto, "override_pipeline", ResourceImpl::override_pipeline);
    MAA_BIND_FUNC(proto, "override_next", ResourceImpl::override_next);
    MAA_BIND_FUNC(proto, "get_node_data", ResourceImpl::get_node_data);
    MAA_BIND_FUNC(proto, "clear", ResourceImpl::clear);
    MAA_BIND_FUNC(proto, "status", ResourceImpl::status);
    MAA_BIND_FUNC(proto, "wait", ResourceImpl::wait);
    MAA_BIND_GETTER(proto, "loaded", ResourceImpl::get_loaded);
    MAA_BIND_GETTER(proto, "hash", ResourceImpl::get_hash);
    MAA_BIND_GETTER(proto, "node_list", ResourceImpl::get_node_list);
}

maajs::ValueType load_resource(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ResourceImpl>::init(env, ctor);
    ExtContext::get(env)->resourceCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
