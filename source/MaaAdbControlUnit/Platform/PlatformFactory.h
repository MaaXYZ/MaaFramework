#pragma once

#include "Conf/Conf.h"

#include "BoostIO.h"

MAA_CTRL_UNIT_NS_BEGIN

using NativeIO = BoostIO;

class PlatformFactory
{
public:
    static std::shared_ptr<PlatformIO> create() { return std::make_shared<NativeIO>(); }
};

MAA_CTRL_UNIT_NS_END
