#include "Utils/IOStream/IOStream.h"

#include "Utils/Time.hpp"

MAA_NS_BEGIN

std::string IOStream::read(duration_t timeout)
{
    return read_some(std::numeric_limits<size_t>::max(), timeout);
}

std::string IOStream::read_some(size_t count, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();
    std::string result;

    while (is_open() && result.size() < count && duration_since(start_time) < timeout) {
        auto data = read_once(count - result.size());
        result.append(std::move(data));
    }

    return result;
}

std::string IOStream::read_until(std::string_view delimiter, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();

    std::string result;

    while (!result.ends_with(delimiter)) {
        auto sub_timeout = timeout - duration_since<duration_t>(start_time);
        if (sub_timeout < duration_t::zero()) {
            break;
        }

        auto sub_str = read_some(1, sub_timeout);
        result.append(std::move(sub_str));
    }

    return result;
}

MAA_NS_END
