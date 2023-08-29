#pragma once

#include "Utils/Format.hpp"

#include <chrono>
#include <string>

#ifdef __APPLE__
#include <ctime>
#include <fcntl.h>
#include <sys/time.h>
#endif

MAA_NS_BEGIN

inline std::string format_now()
{
#ifndef __APPLE__ // Now Apple's compiler cannot build std::chrono::format. 2023/07/21
    return MAA_FMT::format("{}", std::chrono::current_zone()->to_local(
                                     std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())));
#else
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    tm* tm_info = localtime(&nowtime);
    return MAA_FMT::format("{:0>4}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}.{:0>3}", tm_info->tm_year + 1900, tm_info->tm_mon,
                           tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 1000);
#endif
}

inline std::string now_filestem()
{
#ifndef __APPLE__ // Now Apple's compiler cannot build std::chrono::format. 2023/07/21
    return MAA_FMT::format("{:%Y.%m.%d-%H.%M.%S}",
                           std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
#else
    timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    tm* tm_info = localtime(&nowtime);
    return MAA_FMT::format("{:0>4}.{:0>2}.{:0>2}-{:0>2}.{:0>2}.{:0>2}.{}", tm_info->tm_year + 1900, tm_info->tm_mon,
                           tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec);
#endif
}

inline std::chrono::milliseconds duration_since(const std::chrono::steady_clock::time_point& start_time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
}

MAA_NS_END
