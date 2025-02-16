#pragma once

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_AGENT_NS_BEGIN

struct InitMsg
{
    std::string version;
    std::vector<std::string> actions;
    std::vector<std::string> recognitions;

    MEO_JSONIZATION(version, actions, recognitions);
};

struct ContextRunTaskMsg
{
};

MAA_AGENT_NS_END
