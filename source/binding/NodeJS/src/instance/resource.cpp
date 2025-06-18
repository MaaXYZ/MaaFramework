#include "../include/forward.h"

#include "../include/cb.h"
#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

std::optional<Napi::External<ResourceInfo>> resource_create(Napi::Env env, std::optional<Napi::Function> callback)
{
    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaResource* handle = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaResourceCreate(cb, ctx);

    if (handle) {
        return Napi::External<ResourceInfo>::New(env, new ResourceInfo { { handle }, ctx }, &DeleteFinalizer<ResourceInfo*>);
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

void resource_destroy(Napi::External<ResourceInfo> info)
{
    info.Data()->dispose();
}

bool resource_set_option_inference_device(Napi::External<ResourceInfo> info, int32_t id)
{
    return MaaResourceSetOption(info.Data()->handle, MaaResOptionEnum::MaaResOption_InferenceDevice, &id, sizeof(id));
}

bool resource_set_option_inference_execution_provider(Napi::External<ResourceInfo> info, int32_t provider)
{
    return MaaResourceSetOption(
        info.Data()->handle,
        MaaResOptionEnum::MaaResOption_InferenceExecutionProvider,
        &provider,
        sizeof(provider));
}

bool resource_register_custom_recognition(Napi::Env env, Napi::External<ResourceInfo> info, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomRecognizerCallback");
    if (MaaResourceRegisterCustomRecognition(info.Data()->handle, name.c_str(), CustomRecognizerCallback, ctx)) {
        auto old = info.Data()->custom_recognizers[name];
        info.Data()->custom_recognizers[name] = ctx;
        if (old) {
            delete old;
        }
        return true;
    }
    else {
        return false;
    }
}

bool resource_unregister_custom_recognition(Napi::External<ResourceInfo> info, std::string name)
{
    auto& map = info.Data()->custom_recognizers;
    if (MaaResourceUnregisterCustomRecognition(info.Data()->handle, name.c_str())) {
        delete map[name];
        map.erase(name);
        return true;
    }
    else {
        return false;
    }
}

bool resource_clear_custom_recognition(Napi::External<ResourceInfo> info)
{
    if (MaaResourceClearCustomRecognition(info.Data()->handle)) {
        info.Data()->ClearRecos();
        return true;
    }
    else {
        return false;
    }
}

bool resource_register_custom_action(Napi::Env env, Napi::External<ResourceInfo> info, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomActionCallback");
    if (MaaResourceRegisterCustomAction(info.Data()->handle, name.c_str(), CustomActionCallback, ctx)) {
        auto old = info.Data()->custom_actions[name];
        info.Data()->custom_actions[name] = ctx;
        if (old) {
            delete old;
        }
        return true;
    }
    else {
        return false;
    }
}

bool resource_unregister_custom_action(Napi::External<ResourceInfo> info, std::string name)
{
    auto& map = info.Data()->custom_actions;
    if (MaaResourceUnregisterCustomAction(info.Data()->handle, name.c_str())) {
        delete map[name];
        map.erase(name);
        return true;
    }
    else {
        return false;
    }
}

bool resource_clear_custom_action(Napi::External<ResourceInfo> info)
{
    if (MaaResourceClearCustomAction(info.Data()->handle)) {
        info.Data()->ClearActs();
        return true;
    }
    else {
        return false;
    }
}

MaaResId resource_post_bundle(Napi::External<ResourceInfo> info, std::string path)
{
    return MaaResourcePostBundle(info.Data()->handle, path.c_str());
}

bool resource_override_pipeline(Napi::External<ResourceInfo> info, std::string pipeline_override)
{
    return MaaResourceOverridePipeline(info.Data()->handle, pipeline_override.c_str());
}

bool resource_override_next(Napi::External<ResourceInfo> info, std::string node_name, std::vector<std::string> next_list)
{
    StringListBuffer buffer;
    buffer.set_vector(next_list, [](auto str) {
        StringBuffer buf;
        buf.set(str);
        return buf;
    });
    return MaaResourceOverrideNext(info.Data()->handle, node_name.c_str(), buffer);
}

std::optional<std::string> resource_get_node_data(Napi::External<ResourceInfo> info, std::string node_name)
{
    StringBuffer buffer;
    if (MaaResourceGetNodeData(info.Data()->handle, node_name.c_str(), buffer)) {
        return buffer.str();
    }
    else {
        return std::nullopt;
    }
}

bool resource_clear(Napi::External<ResourceInfo> info)
{
    return MaaResourceClear(info.Data()->handle);
}

MaaStatus resource_status(Napi::External<ResourceInfo> info, MaaResId id)
{
    return MaaResourceStatus(info.Data()->handle, id);
}

Napi::Promise resource_wait(Napi::Env env, Napi::External<ResourceInfo> info, MaaResId id)
{
    auto handle = info.Data()->handle;
    auto worker = new SimpleAsyncWork<MaaStatus, "resource_wait">(env, [handle, id]() { return MaaResourceWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool resource_loaded(Napi::External<ResourceInfo> info)
{
    return MaaResourceLoaded(info.Data()->handle);
}

std::optional<std::string> resource_get_hash(Napi::External<ResourceInfo> info)
{
    StringBuffer buffer;
    auto ret = MaaResourceGetHash(info.Data()->handle, buffer);
    if (ret) {
        return Napi::String::New(info.Env(), buffer.str());
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::vector<std::string>> resource_get_node_list(Napi::External<ResourceInfo> info)
{
    StringListBuffer buffer;
    auto ret = MaaResourceGetNodeList(info.Data()->handle, buffer);
    if (ret) {
        return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
    }
    else {
        return std::nullopt;
    }
}

void load_instance_resource(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(resource_create);
    BIND(resource_destroy);
    BIND(resource_set_option_inference_device);
    BIND(resource_set_option_inference_execution_provider);
    BIND(resource_register_custom_recognition);
    BIND(resource_unregister_custom_recognition);
    BIND(resource_clear_custom_recognition);
    BIND(resource_register_custom_action);
    BIND(resource_unregister_custom_action);
    BIND(resource_clear_custom_action);
    BIND(resource_post_bundle);
    BIND(resource_override_pipeline);
    BIND(resource_override_next);
    BIND(resource_get_node_data);
    BIND(resource_clear);
    BIND(resource_status);
    BIND(resource_wait);
    BIND(resource_loaded);
    BIND(resource_get_hash);
    BIND(resource_get_node_list);
}
