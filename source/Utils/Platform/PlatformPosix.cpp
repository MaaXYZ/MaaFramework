#if __has_include(<unistd.h>)
#include "PlatformPosix.h"
#include "Platform.h"

#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

MAA_PLATFORM_NS_BEGIN

os_library_handle library_load(const os_string& path)
{
    return dlopen(path.c_str(), RTLD_LAZY);
}

os_library_function library_get_address(os_library_handle handle, const std::string& name)
{
    return reinterpret_cast<os_library_function>(dlsym(handle, name.c_str()));
}

void library_free(os_library_handle handle)
{
    dlclose(handle);
}

static size_t get_page_size()
{
    return (size_t)sysconf(_SC_PAGESIZE);
}

const size_t page_size = get_page_size();

void* aligned_alloc(size_t len, size_t align)
{
    return ::aligned_alloc(len, align);
}

void aligned_free(void* ptr)
{
    ::free(ptr);
}

MAA_PLATFORM_NS_END

#endif
