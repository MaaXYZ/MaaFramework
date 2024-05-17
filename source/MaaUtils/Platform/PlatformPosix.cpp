#ifndef _WIN32

#include "Utils/Platform.h"

#include <unistd.h>

MAA_NS_BEGIN

os_string to_osstring(std::string_view utf8_str)
{
    return os_string(utf8_str);
}

std::string from_osstring(os_string_view os_str)
{
    return std::string(os_str);
}

std::string utf8_to_crt(std::string_view utf8_str)
{
    return std::string(utf8_str);
}

std::string path_to_utf8_string(const std::filesystem::path& path)
{
    return path.native();
}

MAA_NS_END

#endif
