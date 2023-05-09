#pragma once

#include "Common/MaaConf.h"
#include "Platform/Platform.h"

MAA_NS_BEGIN

using os_string = MAA_PLATFORM_NS::os_string;

#ifdef _WIN32
static_assert(std::same_as<os_string, std::wstring>);
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
