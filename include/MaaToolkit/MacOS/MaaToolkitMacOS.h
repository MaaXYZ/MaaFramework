// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaBool MaaToolkitMacOSCheckPermission(MaaMacOSPermission perm);
    MAA_TOOLKIT_API MaaBool MaaToolkitMacOSRequestPermission(MaaMacOSPermission perm);
    MAA_TOOLKIT_API MaaBool MaaToolkitMacOSRevealPermissionSettings(MaaMacOSPermission perm);

#ifdef __cplusplus
}
#endif
