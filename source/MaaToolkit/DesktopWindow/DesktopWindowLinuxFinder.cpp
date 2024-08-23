#ifdef __linux__

#include "DesktopWindowLinuxFinder.h"

#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowLinuxFinder::find_all() const
{
    LogError << "Not implemented";
    return {};
}

MAA_TOOLKIT_NS_END

#endif // __linux__
