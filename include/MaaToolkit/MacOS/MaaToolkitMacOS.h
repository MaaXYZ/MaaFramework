// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MaaToolkitMacOSCheckPermission(MaaMacOSPermission perm);
    MaaBool MaaToolkitMacOSRequestPermission(MaaMacOSPermission perm);
    MaaBool MaaToolkitMacOSRevealPermissionSettings(MaaMacOSPermission perm);

#ifdef __cplusplus
}
#endif
