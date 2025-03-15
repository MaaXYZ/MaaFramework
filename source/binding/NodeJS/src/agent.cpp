#include "include/cb.h"
#include "include/info.h"
#include "include/loader.h"
#include "include/utils.h"
#include "include/wrapper.h"

#if defined(MAA_NODE_SERVER)

#include <MaaAgentServer/MaaAgentServerAPI.h>

bool agent_server_register_custom_recognition(Napi::Env env, ExtContextInfo* context, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomRecognizerCallback");
    if (MaaAgentServerRegisterCustomRecognition(name.c_str(), CustomRecognizerCallback, ctx)) {
        context->server.emplace_back(ctx);
        return true;
    }
    else {
        delete ctx;
        return false;
    }
}

bool agent_server_register_custom_action(Napi::Env env, ExtContextInfo* context, std::string name, Napi::Function callback)
{
    auto ctx = new CallbackContext(env, callback, "CustomActionCallback");
    if (MaaAgentServerRegisterCustomAction(name.c_str(), CustomActionCallback, ctx)) {
        context->server.emplace_back(ctx);
        return true;
    }
    else {
        delete ctx;
        return false;
    }
}

bool agent_server_start_up(std::string identifier)
{
    return MaaAgentServerStartUp(identifier.c_str());
}

void agent_server_shut_down()
{
    MaaAgentServerShutDown();
}

void agent_server_join()
{
    MaaAgentServerJoin();
}

void agent_server_detach()
{
    MaaAgentServerDetach();
}

#else

#include <MaaAgentClient/MaaAgentClientAPI.h>

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

#endif

void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
#if defined(MAA_NODE_SERVER)
    BIND(agent_server_register_custom_recognition);
    BIND(agent_server_register_custom_action);
    BIND(agent_server_start_up);
    BIND(agent_server_shut_down);
    BIND(agent_server_join);
    BIND(agent_server_detach);

    exports["AgentRole"] = "server";
#else
    BIND(agent_client_create);
    BIND(agent_client_destroy);
    BIND(agent_client_bind_resource);
    BIND(agent_client_create_socket);
    BIND(agent_client_connect);
    BIND(agent_client_disconnect);

    exports["AgentRole"] = "client";
#endif
}
