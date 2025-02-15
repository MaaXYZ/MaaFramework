#pragma once

#include "MaaAgentClient/MaaAgentClientDef.h"

struct MaaAgentClient
{
public:
    virtual ~MaaAgentClient() = default;

    virtual bool bind_resource(MaaResource* resource) = 0;
    virtual bool start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args) = 0;
};
