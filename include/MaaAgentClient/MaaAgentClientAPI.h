#pragma once

#include "MaaAgentClientDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreateV2(const MaaStringBuffer* identifier);

    MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreateTcp(uint16_t port);

    MAA_AGENT_CLIENT_API void MaaAgentClientDestroy(MaaAgentClient* client);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientIdentifier(MaaAgentClient* client, MaaStringBuffer* identifier);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientBindResource(MaaAgentClient* client, MaaResource* res);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientRegisterResourceSink(MaaAgentClient* client, MaaResource* res);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientRegisterControllerSink(MaaAgentClient* client, MaaController* ctrl);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientRegisterTaskerSink(MaaAgentClient* client, MaaTasker* tasker);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientConnect(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientDisconnect(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientConnected(MaaAgentClient* client);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientAlive(MaaAgentClient* client);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientSetTimeout(MaaAgentClient* client, int64_t milliseconds);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientGetCustomRecognitionList(MaaAgentClient* client, /* out */ MaaStringListBuffer* buffer);
    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientGetCustomActionList(MaaAgentClient* client, /* out */ MaaStringListBuffer* buffer);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientGetCustomControllerList(MaaAgentClient* client, /* out */ MaaStringListBuffer* buffer);

    /// Create a controller for a device advertised by this session. Release it with MaaControllerDestroy.
    /// Disconnecting or destroying the client permanently closes the session for existing controllers.
    MAA_AGENT_CLIENT_API MaaController* MaaAgentClientCreateCustomController(MaaAgentClient* client, const char* name);

    MAA_DEPRECATED MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreate();
    MAA_DEPRECATED MAA_AGENT_CLIENT_API MaaBool MaaAgentClientCreateSocket(MaaAgentClient* client, MaaStringBuffer* identifier);

#ifdef __cplusplus
}
#endif
