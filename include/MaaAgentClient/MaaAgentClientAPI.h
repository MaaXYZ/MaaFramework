#pragma once

#include "MaaAgentClientDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreate();

    MAA_AGENT_CLIENT_API void MaaAgentClientDestroy(MaaAgentClient* client);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientBindResource(MaaAgentClient* client, MaaResource* res);

    // if identifier is empty, bind to default address, and output the identifier. otherwise bind to the specified identifier
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientCreateSocket(MaaAgentClient* client, MaaStringBuffer* identifier);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientConnect(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientDisconnect(MaaAgentClient* client);

#ifdef __cplusplus
}
#endif
