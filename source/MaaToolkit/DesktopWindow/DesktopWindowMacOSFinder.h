#pragma once

#if defined(__APPLE__)

#include "DesktopWindowFinder.h"
#include "MaaUtils/SingletonHolder.hpp"

#include "Common/Conf.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowMacOSFinder
    : public DesktopWindowFinder
    , public SingletonHolder<DesktopWindowMacOSFinder>
{
public:
    virtual ~DesktopWindowMacOSFinder() override = default;

    virtual std::vector<DesktopWindow> find_all() const override;
};

MAA_TOOLKIT_NS_END

#endif // __APPLE__
