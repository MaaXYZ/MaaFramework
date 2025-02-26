#pragma once

#include <exception>
#include <format>
#include <string_view>

namespace maapp
{

struct FunctionFailed : public std::exception
{
    FunctionFailed(std::string_view func) { what_ = std::format("function {} failed", func); }

    virtual const char* what() const noexcept override { return what_.c_str(); }

    std::string what_;
};

}
