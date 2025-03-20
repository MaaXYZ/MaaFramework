module;

#include <MaaToolkit/MaaToolkitAPI.h>

export module maa.toolkit;

import maa.core;

// MaaToolkitDef.h

export using ::MaaToolkitAdbDevice;
export using ::MaaToolkitAdbDeviceList;
export using ::MaaToolkitDesktopWindow;
export using ::MaaToolkitDesktopWindowList;

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

// ProjectInterface/MaaToolkitProjectInterface.h

export using ::MaaToolkitProjectInterfaceRegisterCustomRecognition;
export using ::MaaToolkitProjectInterfaceRegisterCustomAction;
export using ::MaaToolkitProjectInterfaceRunCli;
