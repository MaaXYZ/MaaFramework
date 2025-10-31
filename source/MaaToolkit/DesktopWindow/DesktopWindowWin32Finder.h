#pragma once

#if defined(_WIN32)

#include "DesktopWindowFinder.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SingletonHolder.hpp"

#include "Common/Conf.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowWin32Finder
    : public DesktopWindowFinder
    , public SingletonHolder<DesktopWindowWin32Finder>
{
public:
    virtual ~DesktopWindowWin32Finder() override = default;

    virtual std::vector<DesktopWindow> find_all() const override;
};

MAA_TOOLKIT_NS_END

#endif // _WIN32
