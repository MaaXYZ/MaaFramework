#include "resource.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/async.h"
#include "../foundation/callback.h"
#include "../foundation/classes.h"
#include "../foundation/convert.h"
#include "../foundation/utils.h"
#include "buffer.h"
#include "ext.h"

static void ResourceSink(void* resource, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto res = ResourceImpl::locate_object(fn.Env(), reinterpret_cast<MaaResource*>(resource));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    res,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
}

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ResourceImpl)

ResourceImpl::ResourceImpl(MaaResource* res, bool own)
    : resource(res)
    , own(own)
{
}

ResourceImpl::~ResourceImpl()
{
    destroy();
}

void ResourceImpl::destroy()
{
    if (resource) {
        ExtContext::get(env)->resources.del(resource);
    }

    if (own && resource) {
        for (const auto& [id, ctx] : sinks) {
            MaaResourceRemoveSink(resource, id);
            delete ctx;
        }
        MaaResourceDestroy(resource);
    }
    resource = nullptr;
    own = false;
}

MaaSinkId ResourceImpl::add_sink(maajs::FunctionType sink)
{
    auto ctx = new maajs::CallbackContext(sink, "ResourceSink");
    auto id = MaaResourceAddSink(resource, ResourceSink, ctx);
    if (id != MaaInvalidId) {
        sinks[id] = ctx;
    }
    return id;
}

void ResourceImpl::remove_sink(MaaSinkId id)
{
    if (auto it = sinks.find(id); it != sinks.end()) {
        MaaResourceRemoveSink(resource, id);
        delete it->second;
        sinks.erase(it);
    }
}

void ResourceImpl::clear_sinks()
{
    MaaResourceClearSinks(resource);
    for (const auto& [_, ctx] : sinks) {
        delete ctx;
    }
    sinks.clear();
}

maajs::ValueType ResourceImpl::post_bundle(maajs::ValueType self, maajs::EnvType, std::string path)
{
    auto id = MaaResourcePostBundle(resource, path.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

void ResourceImpl::override_pipeline(maajs::ValueType pipeline)
{
    auto str = maajs::JsonStringify(env, pipeline);

    if (!MaaResourceOverridePipeline(resource, str.c_str())) {
        throw maajs::MaaError { "Resource override_pipeline failed" };
    }
}

void ResourceImpl::override_next(std::string node_name, std::vector<std::string> next_list)
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

std::optional<std::string> ResourceImpl::get_node_data(std::string node_name)
{
    StringBuffer buffer;
    if (!MaaResourceGetNodeData(resource, node_name.c_str(), buffer)) {
        return std::nullopt;
    }
    return buffer.str();
}

std::optional<maajs::ValueType> ResourceImpl::get_node_data_parsed(std::string node_name)
{
    auto json = get_node_data(node_name);
    if (!json) {
        return std::nullopt;
    }
    return maajs::JsonParse(env, *json);
}

void ResourceImpl::clear()
{
    MaaResourceClear(resource);
}

MaaStatus ResourceImpl::status(MaaResId id)
{
    return MaaResourceStatus(resource, id);
}

maajs::PromiseType ResourceImpl::wait(MaaResId id)
{
    auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = resource, id]() { return MaaResourceWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool ResourceImpl::get_loaded()
{
    return MaaResourceLoaded(resource);
}

std::optional<std::string> ResourceImpl::get_hash()
{
    StringBuffer buf;
    if (!MaaResourceGetHash(resource, buf)) {
        return std::nullopt;
    }
    return buf.str();
}

std::optional<std::vector<std::string>> ResourceImpl::get_node_list()
{
    StringListBuffer buffer;
    if (!MaaResourceGetNodeList(resource, buffer)) {
        return std::nullopt;
    }

    return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
}

std::string ResourceImpl::to_string()
{
    return std::format(" handle = {:#018x}, {} ", reinterpret_cast<uintptr_t>(resource), own ? "owned" : "rented");
}

maajs::ValueType ResourceImpl::locate_object(maajs::EnvType env, MaaResource* res)
{
    if (auto obj = ExtContext::get(env)->resources.find(res)) {
        return *obj;
    }
    else {
        return maajs::CallCtorHelper(ExtContext::get(env)->resourceCtor, std::to_string(reinterpret_cast<size_t>(res)));
    }
}

void ResourceImpl::init_bind(maajs::ObjectType self)
{
    ExtContext::get(env)->resources.add(resource, self);
}

void ResourceImpl::gc_mark(maajs::NativeMarkerFunc marker)
{
    for (const auto& [_, ctx] : sinks) {
        marker(ctx->fn);
    }
}

ResourceImpl* ResourceImpl::ctor(const maajs::CallbackInfo& info)
{
    if (info.Length() == 1) {
        try {
            MaaResource* handle = reinterpret_cast<MaaResource*>(std::stoull(info[0].As<maajs::StringType>().Utf8Value()));
            return new ResourceImpl { handle, false };
        }
        catch (std::exception&) {
            return nullptr;
        }
    }
    else {
        auto handle = MaaResourceCreate();
        if (!handle) {
            return nullptr;
        }
        return new ResourceImpl { handle, true };
    }
}

void ResourceImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", ResourceImpl::destroy);
    MAA_BIND_FUNC(proto, "add_sink", ResourceImpl::add_sink);
    MAA_BIND_FUNC(proto, "remove_sink", ResourceImpl::remove_sink);
    MAA_BIND_FUNC(proto, "clear_sinks", ResourceImpl::clear_sinks);
    MAA_BIND_FUNC(proto, "post_bundle", ResourceImpl::post_bundle);
    MAA_BIND_FUNC(proto, "override_pipeline", ResourceImpl::override_pipeline);
    MAA_BIND_FUNC(proto, "override_next", ResourceImpl::override_next);
    MAA_BIND_FUNC(proto, "get_node_data", ResourceImpl::get_node_data);
    MAA_BIND_FUNC(proto, "get_node_data_parsed", ResourceImpl::get_node_data_parsed);
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
