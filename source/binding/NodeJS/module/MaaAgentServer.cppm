module;

#include <MaaAgentServer/MaaAgentServerAPI.h>

export module maa.agent.server;

import maa.core;

// MaaAgentServerAPI.h

export using ::MaaAgentServerRegisterCustomRecognition;
export using ::MaaAgentServerRegisterCustomAction;
export using ::MaaAgentServerStartUp;
export using ::MaaAgentServerShutDown;
export using ::MaaAgentServerJoin;
export using ::MaaAgentServerDetach;
