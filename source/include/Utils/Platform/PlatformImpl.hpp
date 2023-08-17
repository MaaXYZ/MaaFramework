#pragma once

#include <cstddef>
#include <filesystem>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef _WIN32

#include "SafeWindows.h"

#include <mbctype.h>

#else //  _WIN32

#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)

#include <libproc.h>
#include <sys/sysctl.h>

#endif

#endif //  _WIN32

#include "Conf/Conf.h"

MAA_PLATFORM_NS_BEGIN

#ifdef _WIN32
using os_pid = DWORD;
using os_library_handle = HMODULE;
#else
using os_pid = pid_t;
using os_library_handle = void*;
#endif

using os_string = std::filesystem::path::string_type;
using os_string_view = std::basic_string_view<os_string::value_type>;
using os_library_function = void (*)();

os_library_handle library_load(const os_string& path); // posix环境下必须以`.`开头, 防止触发ldconfig
os_library_function library_get_address(os_library_handle handle, const std::string& name);
void library_free(os_library_handle handle);

void* aligned_alloc(size_t len, size_t align);
void aligned_free(void* ptr);

struct ProcessInfo
{
    os_pid pid;
    os_string name;
};

std::vector<ProcessInfo> list_process();
os_string get_process_path(os_pid pid);

#include "PlatformImplMacOS.hpp"
#include "PlatformImplPosix.hpp"
#include "PlatformImplWin32.hpp"

// --------- general impl ------------

inline std::filesystem::path path(const os_string_view& os_str)
{
    return std::filesystem::path(os_str);
}

template <typename TElem>
requires std::is_trivial_v<TElem>
class single_page_buffer
{
    TElem* _ptr = nullptr;

public:
    single_page_buffer()
    {
        _ptr = reinterpret_cast<TElem*>(aligned_alloc(page_size, page_size));
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
    inline size_t size() const { return _ptr ? (page_size / sizeof(TElem)) : 0; }
};

MAA_PLATFORM_NS_END
