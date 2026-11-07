#pragma once

// Only logging is replaced. The test compiles the actual HwndUtils.hpp with
// real Win32 types and OpenCV headers, without the unrelated logging runtime.
namespace WindowGeometryTest
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

#define LogInfo                   \
    ::WindowGeometryTest::NullLog \
    {                             \
    }
#define LogDebug                  \
    ::WindowGeometryTest::NullLog \
    {                             \
    }
#define VAR(value) value
