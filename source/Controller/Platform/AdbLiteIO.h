#pragma once

#include "PlatformIO.h"

#ifdef _WIN32
#include "Win32IO.h"
#else
#include "PosixIO.h"
#endif

MAA_CTRL_NS_BEGIN

#ifdef _WIN32
using NativeIO = Win32IO;
#else
using NativeIO = PosixIO;
#endif

MAA_CTRL_NS_END
