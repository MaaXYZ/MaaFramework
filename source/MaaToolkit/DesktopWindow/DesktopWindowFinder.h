#pragma once

#include "DesktopWindowBuffer.hpp"
#include "MaaUtils/Platform.h"

#include "Common/Conf.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowFinder
{
public:
    virtual ~DesktopWindowFinder() = default;

    virtual std::vector<DesktopWindow> find_all() const = 0;
};

MAA_TOOLKIT_NS_END
