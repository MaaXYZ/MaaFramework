#include <chrono>
#include <cstdint>
#include <random>

extern "C" uint32_t sys_now(void)
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

extern "C" uint32_t maa_ios_lwip_random(void)
{
    // 仅由 lwIP 核心线程调用；独立探针提供自己的时钟及随机源。
    static std::mt19937 random(std::random_device { }());
    return random();
}
