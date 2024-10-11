#if !defined(__APPLE__) && !defined(_WIN32)

#include "Utils/GpuOption.h"

MAA_NS_BEGIN

std::optional<int> perfer_gpu()
{
    // TODO
    return std::nullopt;
}

MAA_NS_END

#endif // !defined(__APPLE__) && !defined(_WIN32)
