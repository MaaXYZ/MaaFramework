module;

#include <MaaToolkit/MaaToolkitAPI.h>

export module maa.toolkit;

import maa.core;

// MaaToolkitDef.h

export using ::MaaToolkitAdbDevice;
export using ::MaaToolkitAdbDeviceList;
export using ::MaaToolkitDesktopWindow;
export using ::MaaToolkitDesktopWindowList;
export using ::MaaToolkitGamescopeInstance;
export using ::MaaToolkitGamescopeInstanceList;
export using ::MaaToolkitPortalHelper;

// AdbDevice/MaaToolkitAdbDevice.h

export using ::MaaToolkitAdbDeviceListCreate;
export using ::MaaToolkitAdbDeviceListDestroy;
export using ::MaaToolkitAdbDeviceFind;
export using ::MaaToolkitAdbDeviceFindSpecified;
export using ::MaaToolkitAdbDeviceListSize;
export using ::MaaToolkitAdbDeviceListAt;
export using ::MaaToolkitAdbDeviceGetName;
export using ::MaaToolkitAdbDeviceGetAdbPath;
export using ::MaaToolkitAdbDeviceGetAddress;
export using ::MaaToolkitAdbDeviceGetScreencapMethods;
export using ::MaaToolkitAdbDeviceGetInputMethods;
export using ::MaaToolkitAdbDeviceGetConfig;

// Config/MaaToolkitConfig.h

export using ::MaaToolkitConfigInitOption;

// DesktopWindow/MaaToolkitDesktopWindow.h

export using ::MaaToolkitDesktopWindowListCreate;
export using ::MaaToolkitDesktopWindowListDestroy;
export using ::MaaToolkitDesktopWindowFindAll;
export using ::MaaToolkitDesktopWindowListSize;
export using ::MaaToolkitDesktopWindowListAt;
export using ::MaaToolkitDesktopWindowGetHandle;
export using ::MaaToolkitDesktopWindowGetClassName;
export using ::MaaToolkitDesktopWindowGetWindowName;

// Gamescope/MaaToolkitGamescope.h

export using ::MaaToolkitGamescopeInstanceListCreate;
export using ::MaaToolkitGamescopeInstanceListDestroy;
export using ::MaaToolkitGamescopeInstanceFindAll;
export using ::MaaToolkitGamescopeInstanceListSize;
export using ::MaaToolkitGamescopeInstanceListAt;
export using ::MaaToolkitGamescopeInstanceGetDisplayNo;
export using ::MaaToolkitGamescopeInstanceGetPipeWireNodeId;
export using ::MaaToolkitGamescopeInstanceGetEisSocketPath;

// Portal/MaaToolkitPortal.h

export using ::MaaToolkitPortalHelperCreate;
export using ::MaaToolkitPortalHelperDestroy;
export using ::MaaToolkitPortalHelperOpenStream;
export using ::MaaToolkitPortalHelperGetPersist;
export using ::MaaToolkitPortalHelperSetPersist;
export using ::MaaToolkitPortalHelperGetPipeWireFD;
export using ::MaaToolkitPortalHelperGetPipeWireNodeID;
export using ::MaaToolkitPortalHelperGetRestoreToken;
export using ::MaaToolkitPortalHelperSetRestoreToken;

// MacOS/MaaToolkitMacOSPermission.h

export using ::MaaToolkitMacOSCheckPermission;
export using ::MaaToolkitMacOSRequestPermission;
export using ::MaaToolkitMacOSRevealPermissionSettings;
