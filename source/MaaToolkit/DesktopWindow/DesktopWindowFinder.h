#pragma once

#include "DesktopWindowBuffer.hpp"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowFinder
{
public:
    virtual ~DesktopWindowFinder() = default;

    virtual std::vector<DesktopWindow> find_all() const = 0;
};

MAA_TOOLKIT_NS_END
