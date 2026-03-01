#pragma once

#if defined(__linux__) && defined(ANDROID)

#include "DesktopWindowFinder.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SingletonHolder.hpp"

#include "Common/Conf.h"

MAA_TOOLKIT_NS_BEGIN

class DesktopWindowAndroidFinder
    : public DesktopWindowFinder
    , public SingletonHolder<DesktopWindowAndroidFinder>
{
public:
    virtual ~DesktopWindowAndroidFinder() override = default;

    virtual std::vector<DesktopWindow> find_all() const override;
};

MAA_TOOLKIT_NS_END

#endif // __linux__ && ANDROID
