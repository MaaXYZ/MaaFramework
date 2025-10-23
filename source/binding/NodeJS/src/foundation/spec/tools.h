#pragma once

#include <exception>
#include <string>
#include <vector>

namespace maajs
{

struct MaaError : public std::exception
{
    std::string error;

    MaaError(const std::string& err)
        : error(err)
    {
    }

    MaaError(std::string&& err)
        : error(err)
    {
    }

    virtual const char* what() const noexcept override { return error.c_str(); }
};

template <size_t N>
struct StringHolder
{
    char data[N];

    constexpr StringHolder(const char (&str)[N]) { std::copy(str, str + N, data); }

    template <size_t M>
    constexpr auto operator+(const char (&str)[M])
    {
        StringHolder<N + M - 1> ret;
        std::copy(data, data + N - 1, ret.data);
        std::copy(str.data, str.data + M, ret.data + N - 1);
        return ret;
    }
};

inline std::string JoinString(std::vector<std::string> parts, std::string sep)
{
    std::string result = "";
    bool first = true;
    for (const auto& name : parts) {
        result += first ? name : sep + name;
        first = false;
    }
    return result;
}

}
