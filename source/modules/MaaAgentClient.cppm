module;

#include <MaaAgentClient/MaaAgentClientAPI.h>

export module maa.agent.client;

import maa.core;

// MaaAgentClientDef.h

export using ::MaaAgentClient;

// MaaAgentClientAPI.h

export using ::MaaAgentClientCreateV2;
export using ::MaaAgentClientCreateTcp;
export using ::MaaAgentClientDestroy;
export using ::MaaAgentClientIdentifier;
export using ::MaaAgentClientBindResource;
export using ::MaaAgentClientRegisterResourceSink;
export using ::MaaAgentClientRegisterControllerSink;
export using ::MaaAgentClientRegisterTaskerSink;
export using ::MaaAgentClientConnect;
export using ::MaaAgentClientDisconnect;
export using ::MaaAgentClientConnected;
export using ::MaaAgentClientAlive;
export using ::MaaAgentClientSetTimeout;
export using ::MaaAgentClientGetCustomRecognitionList;
export using ::MaaAgentClientGetCustomActionList;

// Deprecated APIs
export using ::MaaAgentClientCreate;
export using ::MaaAgentClientCreateSocket;
