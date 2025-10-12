#include "../include/forward.h"

#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"
#include "MaaAgentClient/MaaAgentClientAPI.h"

std::optional<Napi::External<AgentClientInfo>> agent_client_create(Napi::Env env, std::optional<std::string> identifier)
{
    StringBuffer buf;
    if (identifier) {
        buf.set(identifier.value());
    }
    auto handle = MaaAgentClientCreateV2(buf);

    if (handle) {
        auto info = Napi::External<AgentClientInfo>::New(env, new AgentClientInfo { { handle } }, &DeleteFinalizer<AgentClientInfo*>);
        return info;
    }
    else {
        return std::nullopt;
    }
}

void agent_client_destroy(Napi::External<AgentClientInfo> info)
{
    info.Data()->dispose();
}

std::optional<std::string> agent_client_identifier(Napi::External<AgentClientInfo> info)
{
    StringBuffer buf;
    if (MaaAgentClientIdentifier(info.Data()->handle, buf)) {
        return buf.str();
    }
    else {
        return std::nullopt;
    }
}

bool agent_client_bind_resource(Napi::External<AgentClientInfo> info, Napi::External<ResourceInfo> res_info)
{
    if (MaaAgentClientBindResource(info.Data()->handle, res_info.Data()->handle)) {
        res_info.Data()->clients.push_back(Napi::Persistent(info));
        return true;
    }
    else {
        return false;
    }
}

Napi::Promise agent_client_connect(Napi::Env env, Napi::External<AgentClientInfo> info)
{
    auto handle = info.Data()->handle;
    auto worker = new SimpleAsyncWork<bool, "agent_client_connect">(env, [handle]() { return MaaAgentClientConnect(handle); });
    worker->Queue();
    return worker->Promise();
}

bool agent_client_disconnect(Napi::External<AgentClientInfo> info)
{
    return MaaAgentClientDisconnect(info.Data()->handle);
}

bool agent_client_connected(Napi::External<AgentClientInfo> info)
{
    return MaaAgentClientConnected(info.Data()->handle);
}

bool agent_client_alive(Napi::External<AgentClientInfo> info)
{
    return MaaAgentClientAlive(info.Data()->handle);
}

bool agent_client_set_timeout(Napi::External<AgentClientInfo> info, uint64_t ms)
{
    return MaaAgentClientSetTimeout(info.Data()->handle, static_cast<int64_t>(ms));
}

void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(agent_client_create);
    BIND(agent_client_destroy);
    BIND(agent_client_identifier);
    BIND(agent_client_bind_resource);
    BIND(agent_client_connect);
    BIND(agent_client_disconnect);
    BIND(agent_client_connected);
    BIND(agent_client_alive);
    BIND(agent_client_set_timeout);

    exports["AgentRole"] = "client";
}
