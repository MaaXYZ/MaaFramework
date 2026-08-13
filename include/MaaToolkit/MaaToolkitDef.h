#pragma once

#include "MaaFramework/MaaDef.h" // IWYU pragma: export

typedef struct MaaToolkitAdbDevice MaaToolkitAdbDevice;
typedef struct MaaToolkitAdbDeviceList MaaToolkitAdbDeviceList;

typedef struct MaaToolkitDesktopWindow MaaToolkitDesktopWindow;
typedef struct MaaToolkitDesktopWindowList MaaToolkitDesktopWindowList;

typedef struct MaaToolkitPortalHelper MaaToolkitPortalHelper;

typedef struct MaaToolkitGamescopeNode MaaToolkitGamescopeNode;
typedef struct MaaToolkitGamescopeNodeList MaaToolkitGamescopeNodeList;
typedef struct MaaToolkitGamescopeEisSocket MaaToolkitGamescopeEisSocket;
typedef struct MaaToolkitGamescopeEisSocketList MaaToolkitGamescopeEisSocketList;

enum MaaMacOSPermissionEnum
{
    MaaMacOSPermissionScreenCapture = 1,
    MaaMacOSPermissionAccessibility = 2
};

typedef int32_t MaaMacOSPermission;
