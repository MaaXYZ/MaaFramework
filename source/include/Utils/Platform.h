#pragma once

#include <filesystem>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"
#else
#include <sys/types.h>
#endif

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

/* string and path */

using os_string = std::filesystem::path::string_type;
using os_string_view = std::basic_string_view<os_string::value_type>;

MAA_UTILS_API os_string to_osstring(std::string_view utf8_str);
MAA_UTILS_API std::string from_osstring(os_string_view os_str);
MAA_UTILS_API std::string utf8_to_crt(std::string_view utf8_str);

inline std::filesystem::path path(std::string_view utf8_str)
{
    return std::filesystem::path(to_osstring(utf8_str)).make_preferred();
}

MAA_UTILS_API std::string path_to_utf8_string(const std::filesystem::path& path);

namespace path_literals
{
inline std::filesystem::path operator""_path(const char* utf8_str, size_t len)
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

MAA_UTILS_API std::set<ProcessInfo> list_processes();
MAA_UTILS_API std::optional<std::filesystem::path> get_process_path(os_pid pid);

MAA_NS_END
