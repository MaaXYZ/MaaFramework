#pragma once

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#endif

#include <chrono>
#include <format>
#include <string>

MAA_NS_BEGIN

inline std::string format_now()
{
#ifndef __APPLE__ // Now Apple's compiler cannot build std::chrono::format. 2023/07/21
    return std::format(
        "{}",
        std::chrono::current_zone()->to_local(
            std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())));
#else
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    tm* tm_info = localtime(&nowtime);
    return std::format(
        "{:0>4}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}.{:0>3}",
        tm_info->tm_year + 1900,
        tm_info->tm_mon,
        tm_info->tm_mday,
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        tv.tv_usec / 1000);
#endif
}

inline std::string format_now_for_filename()
{
#ifndef __APPLE__ // Now Apple's compiler cannot build std::chrono::format. 2023/07/21
    return std::format(
        "{:%Y.%m.%d-%H.%M.%S}",
        std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
#else
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    tm* tm_info = localtime(&nowtime);
    return std::format(
        "{:0>4}.{:0>2}.{:0>2}-{:0>2}.{:0>2}.{:0>2}.{}",
        tm_info->tm_year + 1900,
        tm_info->tm_mon,
        tm_info->tm_mday,
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec,
        tv.tv_usec);
#endif
}

template <typename duration_t = std::chrono::milliseconds>
inline duration_t duration_since(const std::chrono::steady_clock::time_point& start_time)
{
    return std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time);
}

MAA_NS_END