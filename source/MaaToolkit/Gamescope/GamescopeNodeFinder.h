#pragma once

#include <vector>

#include "Common/Conf.h"
#include "Gamescope/GamescopeNodeBuffer.hpp"
#include "MaaToolkit/Gamescope/MaaToolkitGamescope.h"

MAA_TOOLKIT_NS_BEGIN

class GamescopeNodeFinder
{
public:
    static GamescopeNodeFinder& get_instance();

    std::vector<GamescopeNode> find_all();

private:
    GamescopeNodeFinder() = default;
};

MAA_TOOLKIT_NS_END
