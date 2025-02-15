#include "MaaAgentClient/MaaAgentClientAPI.h"

#include "Client/AgentClient.h"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaAgentClient* MaaAgentClientCreate(MaaNotificationCallback notify, void* notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    return new MAA_AGENT_CLIENT_NS::AgentClient(notify, notify_trans_arg);
}

void MaaAgentClientDestroy(MaaAgentClient* client)
{
    LogFunc << VAR_VOIDP(client);

    if (client == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete client;
}

MaaBool MaaAgentClientBindResource(MaaAgentClient* client, MaaResource* res)
{
    LogFunc << VAR_VOIDP(client) << VAR_VOIDP(res);

    if (!client || !res) {
        LogError << "handle is null";
        return false;
    }

    return client->bind_resource(res);
}

MaaBool MaaAgentClientStartChild(MaaAgentClient* client, const char* child_exec, const MaaStringListBuffer* child_args)
{
    LogFunc << VAR_VOIDP(client) << VAR(child_exec) << VAR(child_args);

    if (!client) {
        LogError << "handle is null";
        return false;
    }

    std::vector<std::string> args;

    if (child_args) {
        size_t size = child_args->size();
        for (size_t i = 0; i < size; ++i) {
            args.emplace_back(child_args->at(i).get());
        }
    }

    return client->start_clild(MAA_NS::path(child_exec), args);
}
