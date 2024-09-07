#include "Impl/PIClient.h"
#include "Impl/PIRuntime.h"
#include "MaaPI/MaaPIAPI.h"

#include "Buffer/BufferTypes.hpp"
#include "Impl/PIConfig.h"
#include "Impl/PIData.h"

MaaPIData* MaaPIDataCreate()
{
    return new MAA_PI_NS::PIData();
}

void MaaPIDataDestroy(MaaPIData* data)
{
    if (data == nullptr) {
        return;
    }

    delete data;
}

MaaBool MaaPIDataLoad(MaaPIData* data, const char* json, const char* path)
{
    if (data == nullptr) {
        return false;
    }

    return data->load(json, path);
}

MaaPIConfig* MaaPIConfigCreate()
{
    return new MAA_PI_NS::PIConfig;
}

void MaaPIConfigDestroy(MaaPIConfig* cfg)
{
    if (cfg == nullptr) {
        return;
    }

    delete cfg;
}

MaaBool MaaPIConfigLoad(MaaPIConfig* cfg, MaaPIData* data, const char* json)
{
    return cfg->load(data, json);
}

MaaBool MaaPIConfigGenDef(MaaPIConfig* cfg, MaaPIData* data)
{
    return cfg->gen_default(data);
}

MaaBool MaaPIConfigSave(MaaPIConfig* cfg, MaaStringBuffer* json)
{
    std::string result;
    if (cfg->save(result)) {
        json->set(std::move(result));
        return true;
    }
    else {
        return false;
    }
}

MaaPIRuntime* MaaPIRuntimeCreate()
{
    return new MAA_PI_NS::PIRuntime();
}

void MaaPIRuntimeDestroy(MaaPIRuntime* rt)
{
    if (!rt) {
        return;
    }

    delete rt;
}

MaaController* MaaPIRuntimeGetController(MaaPIRuntime* rt)
{
    if (!rt) {
        return nullptr;
    }

    return rt->get_controller();
}

MaaResource* MaaPIRuntimeGetResource(MaaPIRuntime* rt)
{
    if (!rt) {
        return nullptr;
    }

    return rt->get_resource();
}

MaaTasker* MaaPIRuntimeGetTasker(MaaPIRuntime* rt)
{
    if (!rt) {
        return nullptr;
    }

    return rt->get_tasker();
}

MaaBool MaaPIRuntimeBind(MaaPIRuntime* rt, MaaPIData* data, MaaPIConfig* cfg)
{
    if (!rt || !data || !cfg) {
        return false;
    }

    return rt->bind(data, cfg);
}

MaaBool MaaPIRuntimeSetup(MaaPIRuntime* rt, MaaNotificationCallback cb, void* cb_arg)
{
    if (!rt) {
        return false;
    }

    return rt->setup(cb, cb_arg);
}

MaaPIClient* MaaPIClientCreate(const char* locale, MaaPIClientHandler handler, void* handler_arg)
{
    if (!handler) {
        return nullptr;
    }

    if (!locale) {
        locale = "en-US";
    }

    auto client = new MAA_PI_NS::PIClient();
    client->setup(locale, handler, handler_arg);
    return client;
}

void MaaPIClientDestroy(MaaPIClient* client)
{
    if (!client) {
        return;
    }

    delete client;
}

MaaBool MaaPIClientPerform(MaaPIClient* client, MaaPIRuntime* rt, MaaPIClientAction action)
{
    if (!client || !rt) {
        return false;
    }

    return client->perform(rt, action);
}
