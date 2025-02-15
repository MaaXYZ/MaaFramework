#pragma once

#include "MaaAgentClientDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_CLIENT_API MaaAgentClient* MaaAgentClientCreate(MaaNotificationCallback notify, void* notify_trans_arg);

    MAA_AGENT_CLIENT_API void MaaAgentClientDestroy(MaaAgentClient* client);

    MAA_AGENT_CLIENT_API MaaBool
        MaaAgentClientStartChild(MaaAgentClient* client, const char* child_exec, const MaaStringListBuffer* child_args);

    MAA_AGENT_CLIENT_API MaaBool MaaAgentClientBindResource(MaaAgentClient* client, MaaResource* res);

#ifdef __cplusplus
}
#endif
