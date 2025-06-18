#include "MaaAgentClient/MaaAgentClientAPI.h"

#include "Client/AgentClient.h"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaAgentClient* MaaAgentClientCreate()
{
    LogError << "Deprecated API, use MaaAgentClientCreateV2 instead";

    return new MAA_AGENT_CLIENT_NS::AgentClient;
}

MaaAgentClient* MaaAgentClientCreateV2(const MaaStringBuffer* identifier)
{
    LogFunc << VAR(identifier);

    return new MAA_AGENT_CLIENT_NS::AgentClient(identifier ? identifier->get() : "");
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

MaaBool MaaAgentClientIdentifier(MaaAgentClient* client, MaaStringBuffer* identifier)
{
    LogFunc << VAR_VOIDP(client) << VAR_VOIDP(identifier);

    if (!client || !identifier) {
        LogError << "handle is null";
        return false;
    }

    identifier->set(client->identifier());
    return true;
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
    LogError << "Deprecated API, use MaaAgentClientCreateV2 instead";

    LogFunc << VAR_VOIDP(client) << VAR_VOIDP(identifier);

    if (!client || !identifier) {
        LogError << "handle is null";
        return false;
    }

    std::string id = client->create_socket(identifier->get());

    identifier->set(std::move(id));

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

MaaBool MaaAgentClientDisconnect(MaaAgentClient* client)
{
    LogFunc << VAR_VOIDP(client);

    if (!client) {
        LogError << "handle is null";
        return false;
    }

    return client->disconnect();
}

MaaBool MaaAgentClientConnected(MaaAgentClient* client)
{
    if (!client) {
        LogError << "handle is null";
        return false;
    }

    return client->connected();
}

MaaBool MaaAgentClientAlive(MaaAgentClient* client)
{
    if (!client) {
        LogError << "handle is null";
        return false;
    }

    return client->alive();
}

MaaBool MaaAgentClientSetTimeout(MaaAgentClient* client, int64_t milliseconds)
{
    LogFunc << VAR_VOIDP(client) << VAR_VOIDP(milliseconds);

    if (!client) {
        LogError << "handle is null";
        return false;
    }

    std::chrono::milliseconds timeout = milliseconds < 0 ? std::chrono::milliseconds::max() : std::chrono::milliseconds(milliseconds);
    client->set_timeout(timeout);
    return true;
}
