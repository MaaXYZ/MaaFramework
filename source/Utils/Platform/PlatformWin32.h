#pragma once
#ifdef _WIN32

#include "Common/MaaConf.h"

#include "SafeWindows.h"

#define MAA_PLATFORM_WIN_NS MAA_NS::platform_ns::win32
#define MAA_PLATFORM_WIN_NS_BEGIN \
    namespace MAA_PLATFORM_WIN_NS \
    {
#define MAA_PLATFORM_WIN_NS_END }

MAA_PLATFORM_WIN_NS_BEGIN

bool CreateOverlappablePipe(HANDLE* read, HANDLE* write, SECURITY_ATTRIBUTES* secattr_read,
                            SECURITY_ATTRIBUTES* secattr_write, DWORD bufsize, bool overlapped_read,
                            bool overlapped_write);

MAA_PLATFORM_WIN_NS_END

#endif
