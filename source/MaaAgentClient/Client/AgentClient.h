#pragma once

#include <filesystem>

#include "API/MaaAgentClientTypes.h"
#include "Conf/Conf.h"
#include "Utils/MessageNotifier.hpp"

MAA_AGENT_CLIENT_NS_BEGIN

class AgentClient : public MaaAgentClient
{
public:
    AgentClient(MaaNotificationCallback notify, void* notify_trans_arg);
    virtual ~AgentClient() override = default;

    virtual bool start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args) override;
    virtual bool bind_resource(MaaResource* resource) override;

private:
    MaaResource* resource_ = nullptr;
    MessageNotifier notifier_;
};

MAA_AGENT_CLIENT_NS_END
