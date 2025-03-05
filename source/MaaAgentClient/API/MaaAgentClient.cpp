#include "MaaAgentClient/MaaAgentClientAPI.h"

#include "Client/AgentClient.h"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaAgentClient* MaaAgentClientCreate()
{
    LogFunc;

    return new MAA_AGENT_CLIENT_NS::AgentClient;
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

MaaBool MaaAgentClientCreateSocket(MaaAgentClient* client, MaaStringBuffer* identifier)
{
    LogFunc << VAR_VOIDP(client) << VAR_VOIDP(identifier);

    if (!client || !identifier) {
        LogError << "handle is null";
        return false;
    }

    auto ret = client->create_socket(identifier->get());
    if (!ret) {
        LogError << "failed to bind socket";
        return false;
    }

    identifier->set(*ret);

    return true;
}

MaaBool MaaAgentClientConnect(MaaAgentClient* client)
{
    LogFunc << VAR_VOIDP(client);

    if (!client) {
        LogError << "handle is null";
        return false;
    }

    return client->connect();
}
