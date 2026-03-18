#if !defined(__APPLE__)

#include "MaaToolkit/MacOS/MaaToolkitMacOS.h"

#include "MaaUtils/Logger.h"

MaaBool MaaToolkitMacOSCheckPermission(MaaMacOSPermission perm)
{
    LogError << "This API" << __FUNCTION__ << "is only available on MacOS";
    std::ignore = perm;
    return false;
}

MaaBool MaaToolkitMacOSRequestPermission(MaaMacOSPermission perm)
{
    LogError << "This API" << __FUNCTION__ << "is only available on MacOS";
    std::ignore = perm;
    return false;
}

MaaBool MaaToolkitMacOSRevealPermissionSettings(MaaMacOSPermission perm)
{
    LogError << "This API" << __FUNCTION__ << "is only available on MacOS";
    std::ignore = perm;
    return false;
}

#endif
