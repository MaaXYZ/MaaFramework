#pragma once

#include "Conf/Conf.h"

#include <filesystem>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

#ifdef _WIN32
#include "SafeWindows.hpp"
#else
#include <sys/types.h>
#endif

#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

/* string and path */

using os_string = std::filesystem::path::string_type;
using os_string_view = std::basic_string_view<os_string::value_type>;

os_string MAA_UTILS_API to_osstring(std::string_view utf8_str);
std::string MAA_UTILS_API from_osstring(os_string_view os_str);

inline std::filesystem::path path(std::string_view utf8_str)
{
    return std::filesystem::path(to_osstring(utf8_str)).make_preferred();
}
std::string MAA_UTILS_API path_to_utf8_string(const std::filesystem::path& path);
std::string MAA_UTILS_API path_to_ansi_string(const std::filesystem::path& path);
std::string MAA_UTILS_API path_to_crt_string(const std::filesystem::path& path);

namespace path_literals
{
inline std::filesystem::path operator"" _path(const char* utf8_str, size_t len)
{
    return path(std::string_view(utf8_str, len));
}
}

/* process */

#ifdef _WIN32
using os_pid = DWORD;
#else
using os_pid = pid_t;
#endif

struct ProcessInfo
{
    os_pid pid = 0;
    std::string name;

    bool operator<(const ProcessInfo& rhs) const { return pid < rhs.pid; }
    bool operator==(const ProcessInfo& rhs) const { return pid == rhs.pid; }
};

inline std::ostream& operator<<(std::ostream& os, const ProcessInfo& info)
{
    return os << info.pid << " " << info.name;
}

std::set<ProcessInfo> MAA_UTILS_API list_processes();
std::optional<std::filesystem::path> MAA_UTILS_API get_process_path(os_pid pid);

MAA_NS_END
