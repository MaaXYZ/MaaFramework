#ifndef _WIN32
#include "Utils/Runtime.h"

#include <dlfcn.h>

#include "Utils/Platform.h"

MAA_NS_BEGIN

void init_library_dir();

static std::filesystem::path s_library_dir_cache;

const std::filesystem::path& library_dir()
{
    if (s_library_dir_cache.empty()) {
        init_library_dir();
    }

    return s_library_dir_cache;
}

void init_library_dir()
{
    Dl_info dl_info {};
    if (dladdr((void*)init_library_dir, &dl_info) == 0) {
        return;
    }

    s_library_dir_cache = MAA_NS::path(dl_info.dli_fname).parent_path();
}

MAA_NS_END

#endif
