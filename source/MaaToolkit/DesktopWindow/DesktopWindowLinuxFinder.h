#pragma once

#if defined(__linux__)

#include "DesktopWindowFinder.h"
#include "MaaUtils/SingletonHolder.hpp"

#include "Common/Conf.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowLinuxFinder
    : public DesktopWindowFinder
    , public SingletonHolder<DesktopWindowLinuxFinder>
{
public:
    virtual ~DesktopWindowLinuxFinder() override = default;

    virtual std::vector<DesktopWindow> find_all() const override;
};

MAA_TOOLKIT_NS_END

#endif // __linux__
