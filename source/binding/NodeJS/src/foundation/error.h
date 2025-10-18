#pragma once

#include <exception>
#include <string>

namespace maajs
{

struct MaaError : public std::exception
{
    std::string error;

    MaaError(std::string&& err = "")
        : error(err)
    {
    }

    virtual const char* what() const noexcept override { return error.c_str(); }
};

}
