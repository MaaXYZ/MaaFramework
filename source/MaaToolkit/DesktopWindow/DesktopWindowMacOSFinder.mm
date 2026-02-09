#ifdef __APPLE__

#include "DesktopWindowMacOSFinder.h"

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowMacOSFinder::find_all() const
{
    LogError << "Not implemented";
    return {};
}

MAA_TOOLKIT_NS_END

#endif // __APPLE__
