#include "resource.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "buffer.h"
#include "callback.h"
#include "ext.h"

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
    if (!resource) {
        return;
    }

    ExtContext::get(env)->resources.del(resource);

    for (const auto& [id, ctx] : sinks) {
        MaaResourceRemoveSink(resource, id);
        delete ctx;
    }
    sinks.clear();

    for (const auto& [key, ctx] : recos) {
        MaaResourceUnregisterCustomRecognition(resource, key.c_str());
        delete ctx;
    }
    recos.clear();

    for (const auto& [key, ctx] : acts) {
        MaaResourceUnregisterCustomAction(resource, key.c_str());
        delete ctx;
    }
    acts.clear();

    if (own) {
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
    else {
        delete ctx;
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

void ResourceImpl::set_inference_device(std::variant<std::string, int32_t> id)
{
    int32_t value = 0;
    if (auto str = std::get_if<std::string>(&id)) {
        if (*str == "Auto") {
            value = MaaInferenceDevice_Auto;
        }
        else if (*str == "CPU") {
            value = MaaInferenceDevice_CPU;
        }
        else {
            throw maajs::MaaError { std::format("Resource set inference_device failed, invalid id {}", *str) };
        }
    }
    else if (auto ival = std::get_if<int32_t>(&id)) {
        value = *ival;
    }
    if (!MaaResourceSetOption(resource, MaaResOption_InferenceDevice, &value, sizeof(value))) {
        throw maajs::MaaError { "Resource set inference_device failed" };
    }
}

void ResourceImpl::set_inference_execution_provider(std::string provider)
{
    int32_t value = 0;
    if (provider == "Auto") {
        value = MaaInferenceExecutionProvider_Auto;
    }
    else if (provider == "CPU") {
        value = MaaInferenceExecutionProvider_CPU;
    }
    else if (provider == "DirectML") {
        value = MaaInferenceExecutionProvider_DirectML;
    }
    else if (provider == "CoreML") {
        value = MaaInferenceExecutionProvider_CoreML;
    }
    else if (provider == "CUDA") {
        value = MaaInferenceExecutionProvider_CUDA;
    }
    else {
        throw maajs::MaaError { std::format("Resource set inference_execution_provider failed, invalid provider {}", provider) };
    }
    if (!MaaResourceSetOption(resource, MaaResOption_InferenceExecutionProvider, &value, sizeof(value))) {
        throw maajs::MaaError { "Resource set inference_execution_provider failed" };
    }
}

void ResourceImpl::register_custom_recognition(std::string key, maajs::FunctionType func)
{
    auto ctx = new maajs::CallbackContext(func, "CustomReco");
    if (MaaResourceRegisterCustomRecognition(resource, key.c_str(), CustomReco, ctx)) {
        recos[key] = ctx;
    }
    else {
        delete ctx;
        throw maajs::MaaError { "Resource register_custom_recognition failed" };
    }
}

void ResourceImpl::unregister_custom_recognition(std::string key)
{
    if (!MaaResourceUnregisterCustomRecognition(resource, key.c_str())) {
        throw maajs::MaaError { "Resource unregister_custom_recognition failed" };
    }
    if (auto it = recos.find(key); it != recos.end()) {
        delete it->second;
        recos.erase(it);
    }
}

void ResourceImpl::clear_custom_recognition()
{
    if (!MaaResourceClearCustomRecognition(resource)) {
        throw maajs::MaaError { "Resource clear_custom_recognition failed" };
    }

    for (const auto& [key, ctx] : recos) {
        delete ctx;
    }
    recos.clear();
}

void ResourceImpl::register_custom_action(std::string key, maajs::FunctionType func)
{
    auto ctx = new maajs::CallbackContext(func, "CustomAct");
    if (MaaResourceRegisterCustomAction(resource, key.c_str(), CustomAct, ctx)) {
        acts[key] = ctx;
    }
    else {
        delete ctx;
        throw maajs::MaaError { "Resource register_custom_action failed" };
    }
}

void ResourceImpl::unregister_custom_action(std::string key)
{
    if (!MaaResourceUnregisterCustomAction(resource, key.c_str())) {
        throw maajs::MaaError { "Resource unregister_custom_action failed" };
    }
    if (auto it = acts.find(key); it != acts.end()) {
        delete it->second;
        acts.erase(it);
    }
}

void ResourceImpl::clear_custom_action()
{
    if (!MaaResourceClearCustomAction(resource)) {
        throw maajs::MaaError { "Resource clear_custom_action failed" };
    }

    for (const auto& [key, ctx] : acts) {
        delete ctx;
    }
    acts.clear();
}

maajs::ValueType ResourceImpl::post_bundle(maajs::ValueType self, maajs::EnvType, std::string path)
{
    auto id = MaaResourcePostBundle(resource, path.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ResourceImpl::post_ocr_model(maajs::ValueType self, maajs::EnvType, std::string path)
{
    auto id = MaaResourcePostOcrModel(resource, path.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ResourceImpl::post_pipeline(maajs::ValueType self, maajs::EnvType, std::string path)
{
    auto id = MaaResourcePostPipeline(resource, path.c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->jobCtor, self, id);
}

maajs::ValueType ResourceImpl::post_image(maajs::ValueType self, maajs::EnvType, std::string path)
{
    auto id = MaaResourcePostImage(resource, path.c_str());
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

void ResourceImpl::override_image(std::string image_name, maajs::ArrayBufferType image)
{
    ImageBuffer buffer;
    buffer.set(image);
    if (!MaaResourceOverrideImage(resource, image_name.c_str(), buffer)) {
        throw maajs::MaaError { "Resource override_image failed" };
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

std::optional<std::vector<std::string>> ResourceImpl::get_custom_recognition_list()
{
    StringListBuffer buffer;
    if (!MaaResourceGetCustomRecognitionList(resource, buffer)) {
        return std::nullopt;
    }

    return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
}

std::optional<std::vector<std::string>> ResourceImpl::get_custom_action_list()
{
    StringListBuffer buffer;
    if (!MaaResourceGetCustomActionList(resource, buffer)) {
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
        return maajs::CallCtorHelper(ExtContext::get(env)->resourceCtor, std::to_string(reinterpret_cast<uintptr_t>(res)));
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
    MAA_BIND_FUNC(proto, "register_custom_recognition", ResourceImpl::register_custom_recognition);
    MAA_BIND_FUNC(proto, "unregister_custom_recognition", ResourceImpl::unregister_custom_recognition);
    MAA_BIND_FUNC(proto, "clear_custom_recognition", ResourceImpl::clear_custom_recognition);
    MAA_BIND_FUNC(proto, "register_custom_action", ResourceImpl::register_custom_action);
    MAA_BIND_FUNC(proto, "unregister_custom_action", ResourceImpl::unregister_custom_action);
    MAA_BIND_FUNC(proto, "clear_custom_action", ResourceImpl::clear_custom_action);
    MAA_BIND_FUNC(proto, "post_bundle", ResourceImpl::post_bundle);
    MAA_BIND_FUNC(proto, "post_ocr_model", ResourceImpl::post_ocr_model);
    MAA_BIND_FUNC(proto, "post_pipeline", ResourceImpl::post_pipeline);
    MAA_BIND_FUNC(proto, "post_image", ResourceImpl::post_image);
    MAA_BIND_SETTER(proto, "inference_device", ResourceImpl::set_inference_device);
    MAA_BIND_SETTER(proto, "inference_execution_provider", ResourceImpl::set_inference_execution_provider);
    MAA_BIND_FUNC(proto, "override_pipeline", ResourceImpl::override_pipeline);
    MAA_BIND_FUNC(proto, "override_next", ResourceImpl::override_next);
    MAA_BIND_FUNC(proto, "override_image", ResourceImpl::override_image);
    MAA_BIND_FUNC(proto, "get_node_data", ResourceImpl::get_node_data);
    MAA_BIND_FUNC(proto, "get_node_data_parsed", ResourceImpl::get_node_data_parsed);
    MAA_BIND_FUNC(proto, "clear", ResourceImpl::clear);
    MAA_BIND_FUNC(proto, "status", ResourceImpl::status);
    MAA_BIND_FUNC(proto, "wait", ResourceImpl::wait);
    MAA_BIND_GETTER(proto, "loaded", ResourceImpl::get_loaded);
    MAA_BIND_GETTER(proto, "hash", ResourceImpl::get_hash);
    MAA_BIND_GETTER(proto, "node_list", ResourceImpl::get_node_list);
    MAA_BIND_GETTER(proto, "custom_recognition_list", ResourceImpl::get_custom_recognition_list);
    MAA_BIND_GETTER(proto, "custom_action_list", ResourceImpl::get_custom_action_list);
}

maajs::ValueType load_resource(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ResourceImpl>::init(env, ctor);
    ExtContext::get(env)->resourceCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
