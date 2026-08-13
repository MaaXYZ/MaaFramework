#pragma once

#include <vector>

#include "Common/Conf.h"
#include "Gamescope/GamescopeEisSocketBuffer.hpp"

MAA_TOOLKIT_NS_BEGIN

class GamescopeEisSocketFinder
{
public:
    static GamescopeEisSocketFinder& get_instance();

    std::vector<GamescopeEisSocket> find_all();

private:
    GamescopeEisSocketFinder() = default;
};

MAA_TOOLKIT_NS_END
