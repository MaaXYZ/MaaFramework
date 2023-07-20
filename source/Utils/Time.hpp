#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include "Platform/SafeWindows.h"
#else
#include <ctime>
#include <fcntl.h>
#include <sys/time.h>
#endif

#include "StringMisc.hpp"

MAA_NS_BEGIN

inline std::string format_time()
{
    constexpr std::string_view format = "{:0>4}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}.{:0>3}";

#ifdef _WIN32

    SYSTEMTIME curtime;
    GetLocalTime(&curtime);
    return std::format(format, curtime.wYear, curtime.wMonth, curtime.wDay, curtime.wHour, curtime.wMinute,
                       curtime.wSecond, curtime.wMilliseconds);

#else // ! _WIN32

    timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    tm* tm_info = localtime(&nowtime);
    return std::format(format, tm_info->tm_year + 1900, tm_info->tm_mon, tm_info->tm_mday, tm_info->tm_hour,
                       tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 1000);

#endif // END _WIN32
}

inline std::string time_filestem()
{
    std::string stem = format_time();
    string_replace_all_(stem, { { ":", "-" }, { " ", "_" }, { ".", "-" } });
    return stem;
}

inline std::chrono::milliseconds duration_since(const std::chrono::steady_clock::time_point& start_time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
}

MAA_NS_END
