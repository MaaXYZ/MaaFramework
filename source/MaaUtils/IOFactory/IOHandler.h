#pragma once

#include <chrono>
#include <limits>
#include <string>

#include "Conf/Conf.h"

MAA_NS_BEGIN

class IOHandler
{
public:
    virtual ~IOHandler() = default;

    virtual bool write(const std::string& data) = 0;
    virtual std::string read(std::chrono::milliseconds timeout, size_t max_length = SIZE_MAX) = 0;
    // virtual std::string read_err(std::chrono::milliseconds timeout) = 0;
    // virtual std::string read_err(size_t length) = 0;
};

MAA_NS_END
