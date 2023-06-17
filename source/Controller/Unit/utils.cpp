#define _CRT_SECURE_NO_WARNINGS

#include "Utils.h"
#include "Utils/Logger.hpp"

#include <cstdio>

MAA_CTRL_UNIT_NS_BEGIN

std::string temp_name()
{
    char p[L_tmpnam] = { 0 };
#ifdef _WIN32
    tmpnam(p);
    auto pos = std::string(p).find_last_of("\\/");
    LogInfo << p << pos << (p + pos + 1);
    return p + pos + 1;
#else
    auto _ret __attribute__((unused)) = tmpnam(p);
    return p + sizeof(P_tmpdir);
#endif
}

std::filesystem::path temp_path(const std::string& name)
{
#ifdef _WIN32
    return std::filesystem::temp_directory_path() / name;
#else
    // 虽然可以就用上面那个, 但是不确定这是否是同一个, 保险起见还是用现有的marcro
    return std::filesystem::path(P_tmpdir) / name;
#endif
}

MAA_CTRL_UNIT_NS_END
