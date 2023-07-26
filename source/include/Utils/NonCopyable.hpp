#pragma once

#include "Conf/Conf.h"

MAA_NS_BEGIN

class NonCopyable
{
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = delete;

    NonCopyable operator=(const NonCopyable&) = delete;
    NonCopyable operator=(NonCopyable&&) = delete;

protected:
    NonCopyable() = default;
};

MAA_NS_END
