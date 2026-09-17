#pragma once

#include <vector>

#include "Common/Conf.h"
#include "Gamescope/GamescopeInstanceBuffer.hpp"

MAA_TOOLKIT_NS_BEGIN

class GamescopeFinder
{
public:
    static GamescopeFinder& get_instance();

    std::vector<GamescopeInstance> find_all();

private:
    GamescopeFinder() = default;
};

MAA_TOOLKIT_NS_END
