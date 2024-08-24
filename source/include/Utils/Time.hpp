#pragma once

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#endif

#include <chrono>
#include <format>
#include <string>

#include "Conf/Conf.h"

MAA_NS_BEGIN

inline std::string format_now()
{
    return std::format(
        "{}",
        std::chrono::current_zone()->to_local(
            std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())));
}

inline std::string format_now_for_filename()
{
    return std::format(
        "{:%Y.%m.%d-%H.%M.%S}",
        std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
}

template <typename duration_t = std::chrono::milliseconds>
inline duration_t duration_since(const std::chrono::steady_clock::time_point& start_time)
{
    return std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time);
}

MAA_NS_END
