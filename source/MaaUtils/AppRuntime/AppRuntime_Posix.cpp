#ifndef _WIN32
#include "Utils/AppRuntime.h"

#include <dlfcn.h>

#include "Utils/Platform.h"

MAA_NS_BEGIN

void init_app_dir();

static std::filesystem::path s_app_dir_cache;

const std::filesystem::path& app_dir()
{
    if (s_app_dir_cache.empty()) {
        init_app_dir();
    }

    return s_app_dir_cache;
}

void init_app_dir()
{
    Dl_info dl_info{};
    if (dladdr((void*)init_app_dir, &dl_info) == 0) {
        return;
    }

    s_app_dir_cache = MAA_NS::path(dl_info.dli_fname).parent_path();
}

MAA_NS_END

#endif
