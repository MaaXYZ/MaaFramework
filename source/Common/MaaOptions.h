#pragma once

#include <string_view>

#include "MaaConf.h"

MAA_NS_BEGIN

namespace option
{
namespace global
{
    static constexpr std::string_view LogDir = "LogDir"; // value: path string, default is current directory
}
namespace resource
{
    // TODO
}
namespace controller
{
    static constexpr std::string_view KillAdbOnExit = "KillAdbOnExit"; // value: "0" | "1", default is 0
}
namespace instance
{
    // TODO
}
}

MAA_NS_END
