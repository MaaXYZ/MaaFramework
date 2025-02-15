#include "AgentClient.h"

MAA_AGENT_CLIENT_NS_BEGIN

AgentClient::AgentClient(MaaNotificationCallback notify, void* notify_trans_arg)
    : notifier_(notify, notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);
}

bool AgentClient::start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args)
{
    LogFunc << VAR(child_exec) << VAR(child_args);

    return false;
}

bool AgentClient::bind_resource(MaaResource* resource)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource);

    resource_ = resource;

    return true;
}

MAA_AGENT_CLIENT_NS_END
