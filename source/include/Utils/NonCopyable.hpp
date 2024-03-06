#pragma once

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

MAA_UTILS_API class NonCopyButMovable
{
public:
    NonCopyButMovable(const NonCopyButMovable&) = delete;
    NonCopyButMovable(NonCopyButMovable&&) = default;

    NonCopyButMovable& operator=(const NonCopyButMovable&) = delete;
    NonCopyButMovable& operator=(NonCopyButMovable&&) = default;

protected:
    NonCopyButMovable() = default;
};

MAA_UTILS_API class NonCopyable
{
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = delete;

    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) = delete;

protected:
    NonCopyable() = default;
};

MAA_NS_END
