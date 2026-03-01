#if defined(__linux__) && defined(ANDROID)

#include "DesktopWindowAndroidFinder.h"

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowAndroidFinder::find_all() const
{
    LogError << "Not implemented";
    return {};
}

MAA_TOOLKIT_NS_END

#endif // __linux__ && ANDROID
