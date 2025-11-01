#pragma once

#include <memory>

#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/Platform.h"

#include "Common/Conf.h"

struct MaaAgentClient;

MAA_NS_BEGIN

class AgentClientLibraryHolder : public LibraryHolder<AgentClientLibraryHolder>
{
public:
    static std::shared_ptr<MaaAgentClient> create_agent_client();

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaAgentClient");
    inline static const std::string create_func_name_ = "MaaAgentClientCreateV2";
    inline static const std::string destroy_func_name_ = "MaaAgentClientDestroy";
};

MAA_NS_END
