#ifndef _WIN32

#include "Platform.h"

os_string to_osstring(std::string_view utf8_str)
{
    return utf8_str;
}

std::string from_osstring(os_string_view os_str)
{
    return os_str;
}

std::string path_to_utf8_string(const std::filesystem::path& path)
{
    return path.native();
}

std::string path_to_ansi_string(const std::filesystem::path& path)
{
    return path.native();
}

std::string path_to_crt_string(const std::filesystem::path& path)
{
    return path.native();
}

size_t get_page_size()
{
    return (size_t)sysconf(_SC_PAGESIZE);
}

void* aligned_alloc(size_t len, size_t align)
{
    return ::aligned_alloc(len, align);
}

void aligned_free(void* ptr)
{
    ::free(ptr);
}

#endif
