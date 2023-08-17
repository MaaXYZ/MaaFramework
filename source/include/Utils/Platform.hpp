#pragma once

#include "Conf/Conf.h"
#include "Platform/PlatformImpl.hpp"

MAA_NS_BEGIN

using os_pid = MAA_PLATFORM_NS::os_pid;
using os_string = MAA_PLATFORM_NS::os_string;

#ifdef _WIN32
static_assert(std::same_as<os_string, std::wstring>);

using MAA_PLATFORM_NS::args_to_cmd;
using MAA_PLATFORM_NS::cmd_to_args;
#else
static_assert(std::same_as<os_string, std::string>);
#endif

using MAA_PLATFORM_NS::from_osstring;
using MAA_PLATFORM_NS::path;
using MAA_PLATFORM_NS::path_to_ansi_string;
using MAA_PLATFORM_NS::path_to_crt_string;
using MAA_PLATFORM_NS::path_to_utf8_string;
using MAA_PLATFORM_NS::to_osstring;

namespace path_literals
{
inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
{
    return path(std::string_view(utf8_str, len));
}
}

MAA_NS_END
