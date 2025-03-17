module;

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "../include/macro.h"

export module maa.nodejs.agent.server;

import napi;
import stdmock;

import maa.nodejs.cb;
import maa.nodejs.info;
import maa.nodejs.utils;
import maa.nodejs.wrapper;

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

export void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(agent_server_register_custom_recognition);
    BIND(agent_server_register_custom_action);
    BIND(agent_server_start_up);
    BIND(agent_server_shut_down);
    BIND(agent_server_join);
    BIND(agent_server_detach);

    exports["AgentRole"] = "server";
}
