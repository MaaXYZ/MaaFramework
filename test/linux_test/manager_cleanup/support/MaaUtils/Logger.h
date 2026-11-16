#pragma once

namespace ManagerCleanupTest
{
struct NullLog
{
    template <typename T>
    const NullLog& operator<<(const T&) const
    {
        return *this;
    }
};
}

#define LogError ::ManagerCleanupTest::NullLog { }
#define LogFunc ((void)0)
#define VAR(value) value
