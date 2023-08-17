#pragma once

#include "Conf/Conf.h"

#include <filesystem>
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
    return std::filesystem::path(to_osstring(utf8_str));
}
std::string MAA_UTILS_API path_to_utf8_string(const std::filesystem::path& path);
std::string MAA_UTILS_API path_to_ansi_string(const std::filesystem::path& path);
std::string MAA_UTILS_API path_to_crt_string(const std::filesystem::path& path);

namespace path_literals
{
inline std::filesystem::path operator"" _p(const char* utf8_str, size_t len)
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
    os_pid pid;
    std::string name;

    bool operator<(const ProcessInfo& rhs) const { return pid < rhs.pid; }
    bool operator==(const ProcessInfo& rhs) const { return pid == rhs.pid; }
};

inline std::ostream& operator<<(std::ostream& os, const ProcessInfo& info)
{
    return os << info.pid << " " << info.name;
}

std::set<ProcessInfo> MAA_UTILS_API list_process();
os_string MAA_UTILS_API get_process_path(os_pid pid);

/* single_page_buffer */

size_t MAA_UTILS_API get_page_size();
using void_p = void*;
void_p MAA_UTILS_API aligned_alloc(size_t len, size_t align);
void MAA_UTILS_API aligned_free(void* ptr);

template <typename TElem>
requires std::is_trivial_v<TElem>
class single_page_buffer
{
    TElem* _ptr = nullptr;

public:
    single_page_buffer()
    {
        _ptr = reinterpret_cast<TElem*>(aligned_alloc(page_size_, page_size_));
        if (!_ptr) throw std::bad_alloc();
    }

    explicit single_page_buffer(std::nullptr_t) {}

    ~single_page_buffer()
    {
        if (_ptr) aligned_free(reinterpret_cast<void*>(_ptr));
    }

    // disable copy construct
    single_page_buffer(const single_page_buffer&) = delete;
    single_page_buffer& operator=(const single_page_buffer&) = delete;

    inline single_page_buffer(single_page_buffer&& other) noexcept { std::swap(_ptr, other._ptr); }
    inline single_page_buffer& operator=(single_page_buffer&& other) noexcept
    {
        if (_ptr) {
            aligned_free(reinterpret_cast<void*>(_ptr));
            _ptr = nullptr;
        }
        std::swap(_ptr, other._ptr);
        return *this;
    }

    inline TElem* get() const { return _ptr; }
    inline size_t size() const { return _ptr ? (page_size_ / sizeof(TElem)) : 0; }

private:
    inline static const size_t page_size_ = get_page_size();
};

MAA_NS_END
