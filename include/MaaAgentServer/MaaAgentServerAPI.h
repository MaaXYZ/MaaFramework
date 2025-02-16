#pragma once

#include "MaaAgentServerDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_SERVER_API MaaBool MaaAgentServerStartUp(const MaaStringListBuffer* args);
    MAA_AGENT_SERVER_API void MaaAgentServerShutDown();

    MAA_AGENT_SERVER_API MaaBool
        MaaAgentServerRegisterCustomRecognition(const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg);

    MAA_AGENT_SERVER_API MaaBool MaaAgentServerRegisterCustomAction(const char* name, MaaCustomActionCallback action, void* trans_arg);

#ifdef __cplusplus
}
#endif
