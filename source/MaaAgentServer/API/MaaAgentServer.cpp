#include "MaaAgentServer/MaaAgentServerAPI.h"

#include "Server/AgentServer.h"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaBool MaaAgentServerStartUp(const char* identifier)
{
    LogFunc << VAR(identifier);

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().start_up(identifier);
}

void MaaAgentServerShutDown()
{
    LogFunc;

    MAA_AGENT_SERVER_NS::AgentServer::get_instance().shut_down();
}

void MaaAgentServerJoin()
{
    LogFunc;

    MAA_AGENT_SERVER_NS::AgentServer::get_instance().join();
}

void MaaAgentServerDetach()
{
    LogFunc;

    MAA_AGENT_SERVER_NS::AgentServer::get_instance().detach();
}

MaaBool MaaAgentServerRegisterCustomRecognition(const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogFunc << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (!name || !recognition) {
        LogError << "name or recognition is null";
        return false;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().register_custom_recognition(name, recognition, trans_arg);
}

MaaBool MaaAgentServerRegisterCustomAction(const char* name, MaaCustomActionCallback action, void* trans_arg)
{
    LogFunc << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (!name || !action) {
        LogError << "name or action is null";
        return false;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().register_custom_action(name, action, trans_arg);
}
