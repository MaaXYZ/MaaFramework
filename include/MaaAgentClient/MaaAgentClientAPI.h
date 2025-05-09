#pragma once

#include "MaaAgentClientDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreateV2(const MaaStringBuffer* identifier);

    MAA_AGENT_CLIENT_API void MaaAgentClientDestroy(MaaAgentClient* client);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientIdentifier(MaaAgentClient* client, MaaStringBuffer* identifier);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientBindResource(MaaAgentClient* client, MaaResource* res);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientConnect(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientDisconnect(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientConnected(MaaAgentClient* client);

    MAA_DEPRECATED MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreate();
    MAA_DEPRECATED MAA_AGENT_CLIENT_API MaaBool MaaAgentClientCreateSocket(MaaAgentClient* client, MaaStringBuffer* identifier);

#ifdef __cplusplus
}
#endif
