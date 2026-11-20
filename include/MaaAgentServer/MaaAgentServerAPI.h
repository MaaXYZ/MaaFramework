#pragma once

#include "MaaAgentServerDef.h"
#include "MaaFramework/Instance/MaaCustomController.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_AGENT_SERVER_API
    MaaBool MaaAgentServerRegisterCustomRecognition(const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg);

    MAA_AGENT_SERVER_API MaaBool MaaAgentServerRegisterCustomAction(const char* name, MaaCustomActionCallback action, void* trans_arg);

    /// Publish a named device before startup. The table is copied. Keep trans_arg valid until the server exits.
    MAA_AGENT_SERVER_API MaaBool
        MaaAgentServerRegisterCustomController(const char* name, const struct MaaCustomControllerCallbacks* callbacks, void* trans_arg);

    MAA_AGENT_SERVER_API MaaSinkId MaaAgentServerAddResourceSink(MaaEventCallback sink, void* trans_arg);
    MAA_AGENT_SERVER_API MaaSinkId MaaAgentServerAddControllerSink(MaaEventCallback sink, void* trans_arg);
    MAA_AGENT_SERVER_API MaaSinkId MaaAgentServerAddTaskerSink(MaaEventCallback sink, void* trans_arg);
    MAA_AGENT_SERVER_API MaaSinkId MaaAgentServerAddContextSink(MaaEventCallback sink, void* trans_arg);

    MAA_AGENT_SERVER_API MaaBool MaaAgentServerStartUp(const char* identifier);
    MAA_AGENT_SERVER_API void MaaAgentServerShutDown();
    MAA_AGENT_SERVER_API void MaaAgentServerJoin();
    MAA_AGENT_SERVER_API void MaaAgentServerDetach();

#ifdef __cplusplus
}
#endif
