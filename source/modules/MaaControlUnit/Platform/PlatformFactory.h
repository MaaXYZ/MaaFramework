#pragma once

#include "Common/MaaConf.h"

#ifdef _WIN32
#include "Win32IO.h"
#else
#include "PosixIO.h"
#endif

MAA_CTRL_UNIT_NS_BEGIN

#ifdef _WIN32
using NativeIO = Win32IO;
#else
using NativeIO = PosixIO;
#endif

class PlatformFactory
{
public:
    static std::shared_ptr<PlatformIO> create() { return std::make_shared<NativeIO>(); }
};

MAA_CTRL_UNIT_NS_END
