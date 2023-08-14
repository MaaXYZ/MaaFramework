#pragma once

#include "Device/DeviceMgrLinux.h"
#include "Device/DeviceMgrMacOS.h"
#include "Device/DeviceMgrWin32.h"

MAA_TOOLKIT_DEVICE_NS_BEGIN

#if defined(_WIN32)

using DeviceMgr = DeviceMgrWin32;

#elif defined(__linux__)

using DeviceMgr = DeviceMgrLinux;

#elif defined(__APPLE__)

using DeviceMgr = DeviceMgrMacOS;

#endif

MAA_TOOLKIT_DEVICE_NS_END
