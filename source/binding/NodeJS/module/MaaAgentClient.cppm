module;

#include <MaaAgentClient/MaaAgentClientAPI.h>

export module maa.agent.client;

import maa.core;

// MaaAgentClientDef.h

export using ::MaaAgentClient;

// MaaAgentClientAPI.h

export using ::MaaAgentClientCreate;
export using ::MaaAgentClientDestroy;
export using ::MaaAgentClientBindResource;
export using ::MaaAgentClientCreateSocket;
export using ::MaaAgentClientConnect;
export using ::MaaAgentClientDisconnect;
