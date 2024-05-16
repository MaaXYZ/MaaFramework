#pragma once

#include <exception>
#include <format>
#include <string>
#include <string_view>

namespace maa
{

struct Exception : public std::exception
{
};

template <typename Target>
struct ActionInvalidId : public Exception
{
    std::string msg = std::format("{}: MaaInvalidID got in put_action", typeid(Target).name());

    virtual const char* what() const noexcept override { return msg.c_str(); }
};

template <typename Target, typename Handle>
struct NullHandle : public Exception
{
    std::string msg = std::format(
        "{}: nullptr({}) got in ActionHelper",
        typeid(Target).name(),
        typeid(Handle).name());

    virtual const char* what() const noexcept override { return msg.c_str(); }
};

struct FunctionFailed : public Exception
{
    std::string msg;

    FunctionFailed(std::string_view func)
        : msg(std::format("{}: failed", func))
    {
    }

    virtual const char* what() const noexcept override { return msg.c_str(); }
};

}
