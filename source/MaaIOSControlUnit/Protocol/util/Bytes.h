#pragma once

#include <cstdint>

namespace maa::ios::util
{
constexpr uint16_t read_be16(const uint8_t* bytes)
{
    return static_cast<uint16_t>(uint16_t(bytes[0]) << 8 | bytes[1]);
}

constexpr uint32_t read_be32(const uint8_t* bytes)
{
    return uint32_t(bytes[0]) << 24 | uint32_t(bytes[1]) << 16 | uint32_t(bytes[2]) << 8 | bytes[3];
}
} // namespace maa::ios::util
