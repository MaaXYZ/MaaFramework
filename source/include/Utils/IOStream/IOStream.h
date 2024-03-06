#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "BoostIO.hpp"
#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

class MAA_UTILS_API IOStream
{
public:
    using duration_t = std::chrono::milliseconds;

    virtual ~IOStream() = default;

public:
    virtual bool write(std::string_view data) = 0;

    virtual std::string read(duration_t timeout = duration_t::max());
    virtual std::string read_some(size_t count, duration_t timeout = duration_t::max());
    virtual std::string read_until(std::string_view delimiter, duration_t timeout = duration_t::max());

    virtual bool release() = 0;
    virtual bool is_open() const = 0;

protected:
    virtual std::string read_once(size_t max_count) = 0;
};

MAA_NS_END
