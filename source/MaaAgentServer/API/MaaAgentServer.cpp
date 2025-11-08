#include "MaaAgentServer/MaaAgentServerAPI.h"

#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"
#include "Server/AgentServer.h"

MaaBool MaaAgentServerStartUp(const char* identifier)
{
    LogFunc << VAR(identifier);

    if (!identifier) {
        LogError << "identifier is null";
        return false;
    }

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

MaaSinkId MaaAgentServerAddResourceSink(MaaEventCallback sink, void* trans_arg)
{
    LogFunc << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!sink) {
        LogError << "sink is null";
        return MaaInvalidId;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().add_resource_sink(sink, trans_arg);
}

MaaSinkId MaaAgentServerAddControllerSink(MaaEventCallback sink, void* trans_arg)
{
    LogFunc << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!sink) {
        LogError << "sink is null";
        return MaaInvalidId;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().add_controller_sink(sink, trans_arg);
}

MaaSinkId MaaAgentServerAddTaskerSink(MaaEventCallback sink, void* trans_arg)
{
    LogFunc << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!sink) {
        LogError << "sink is null";
        return MaaInvalidId;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().add_tasker_sink(sink, trans_arg);
}

MaaSinkId MaaAgentServerAddContextSink(MaaEventCallback sink, void* trans_arg)
{
    LogFunc << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!sink) {
        LogError << "sink is null";
        return MaaInvalidId;
    }

    return MAA_AGENT_SERVER_NS::AgentServer::get_instance().add_context_sink(sink, trans_arg);
}
