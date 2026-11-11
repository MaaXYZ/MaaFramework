#pragma once

#include <chrono>
#include <cstring>
#include <tuple>

// Keep the real Portal implementation and libdbus; only the logging runtime is
// replaced so the standalone test does not require unrelated MaaDeps libraries.
struct PortalTestLog
{
    template <typename T>
    const PortalTestLog& operator<<(const T&) const
    {
        return *this;
    }
};

#define LogError PortalTestLog {}
