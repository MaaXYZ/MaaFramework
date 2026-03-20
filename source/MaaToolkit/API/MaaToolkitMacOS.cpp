#include "MaaToolkit/MacOS/MaaToolkitMacOS.h"

#include "../MacOS/MacOSPermission.h"
#include "MaaUtils/Logger.h"

MaaBool MaaToolkitMacOSCheckPermission(MaaMacOSPermission perm)
{
#if defined(__APPLE__)
    return MAA_TOOLKIT_NS::MacOSCheckPermission(perm);
#else
    LogError << "This API " << __FUNCTION__ << " is only available on macOS";
    std::ignore = perm;
    return false;
#endif
}

MaaBool MaaToolkitMacOSRequestPermission(MaaMacOSPermission perm)
{
#if defined(__APPLE__)
    return MAA_TOOLKIT_NS::MacOSRequestPermission(perm);
#else
    LogError << "This API " << __FUNCTION__ << " is only available on macOS";
    std::ignore = perm;
    return false;
#endif
}

MaaBool MaaToolkitMacOSRevealPermissionSettings(MaaMacOSPermission perm)
{
#if defined(__APPLE__)
    return MAA_TOOLKIT_NS::MacOSRevealPermissionSettings(perm);
#else
    LogError << "This API " << __FUNCTION__ << " is only available on macOS";
    std::ignore = perm;
    return false;
#endif
}
