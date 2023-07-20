#pragma once

#include <chrono>
#include <format>
#include <string>

MAA_NS_BEGIN

inline std::string format_now()
{
    return std::format("{}", std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()));
}

inline std::string now_filestem()
{
    return std::format("{:%Y.%m.%d-%H.%M.%S}", std::chrono::system_clock::now());
}

inline std::chrono::milliseconds duration_since(const std::chrono::steady_clock::time_point& start_time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
}

MAA_NS_END
