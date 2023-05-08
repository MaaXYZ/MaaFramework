#if __has_include(<unistd.h>)
#include "PlatformPosix.h"
#include "Platform.h"

#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

MAA_PLATFORM_NS_BEGIN

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
