#pragma once

#include "MaaFramework/MaaDef.h" // IWYU pragma: export

typedef struct MaaToolkitAdbDevice MaaToolkitAdbDevice;
typedef struct MaaToolkitAdbDeviceList MaaToolkitAdbDeviceList;

typedef struct MaaToolkitDesktopWindow MaaToolkitDesktopWindow;
typedef struct MaaToolkitDesktopWindowList MaaToolkitDesktopWindowList;

typedef struct MaaToolkitPortalHelper MaaToolkitPortalHelper;

typedef struct MaaToolkitGamescopeInstance MaaToolkitGamescopeInstance;
typedef struct MaaToolkitGamescopeInstanceList MaaToolkitGamescopeInstanceList;

enum MaaMacOSPermissionEnum
{
    MaaMacOSPermissionScreenCapture = 1,
    MaaMacOSPermissionAccessibility = 2
};

typedef int32_t MaaMacOSPermission;
