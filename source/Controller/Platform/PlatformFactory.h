#pragma once

#include "AdbLiteIO.h"
#ifdef _WIN32
#include "Win32IO.h"
#else
#include "PosixIO.h"
#endif

MAA_CTRL_NS_BEGIN

class PlatformFactory
{
public:
    static std::shared_ptr<PlatformIO> create_platform(IPlatformRelatedInstanceInfo* infoProvider, PlatformType type)
    {
        switch (type) {
        // case PlatformType::AdbLite:
        //    return std::make_shared<AdbLiteIO>(infoProvider);
        case PlatformType::Native:
            return std::make_shared<NativeIO>(infoProvider);
        default:
            return nullptr;
        }
    }
};

MAA_CTRL_NS_END
