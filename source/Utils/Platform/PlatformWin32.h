#pragma once
#ifdef _WIN32

#include "SafeWindows.h"

#define MAA_WIN32_NS MAA_NS::win32_ns
#define MAA_WIN32_NS_BEGIN \
    namespace MAA_WIN32_NS \
    {
#define MAA_WIN32_NS_END }

MAA_WIN32_NS_BEGIN

bool CreateOverlappablePipe(HANDLE* read, HANDLE* write, SECURITY_ATTRIBUTES* secattr_read,
                            SECURITY_ATTRIBUTES* secattr_write, DWORD bufsize, bool overlapped_read,
                            bool overlapped_write);

MAA_WIN32_NS_END

#endif
