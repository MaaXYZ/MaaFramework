module;

#include "../include/forward.h"
#include "../include/macro.h"

module maa.nodejs.agent;

import maa.agent.client;
import napi;
import stdmock;

import maa.nodejs.cb;
import maa.nodejs.info;
import maa.nodejs.utils;
import maa.nodejs.wrapper;

extern "C++" void AgentClientDestroy(MaaAgentClient* client)
{
    MaaAgentClientDestroy(client);
}

std::optional<Napi::External<AgentClientInfo>> agent_client_create(Napi::Env env)
{
    auto handle = MaaAgentClientCreate();

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

std::optional<std::string> agent_client_create_socket(Napi::External<AgentClientInfo> info, std::optional<std::string> identifier)
{
    StringBuffer buf;
    if (identifier) {
        buf.set(identifier.value());
    }
    if (MaaAgentClientCreateSocket(info.Data()->handle, buf.buffer)) {
        return buf.str();
    }
    else {
        return std::nullopt;
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

void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(agent_client_create);
    BIND(agent_client_destroy);
    BIND(agent_client_bind_resource);
    BIND(agent_client_create_socket);
    BIND(agent_client_connect);
    BIND(agent_client_disconnect);

    exports["AgentRole"] = "client";
}
