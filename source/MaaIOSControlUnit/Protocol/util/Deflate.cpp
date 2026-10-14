#include "util/Deflate.h"
#include <algorithm>

namespace maa::ios::util
{
namespace
{

constexpr std::size_t kMaxStoredBlock = 65535;
constexpr uint32_t kAdlerMod = 65521;

} // namespace

uint32_t adler32(const uint8_t* data, std::size_t len)
{
    uint32_t a = 1, b = 0;
    for (std::size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % kAdlerMod;
        b = (b + a) % kAdlerMod;
    }
    return b << 16 | a;
}

std::vector<uint8_t> zlib_store(std::vector<uint8_t> data)
{
    std::vector<uint8_t> out;
    out.reserve(data.size() + data.size() / kMaxStoredBlock * 5 + 12);
    out.push_back(0x78);
    out.push_back(0x9c); // CM=8, CINFO=7, FCHECK 使整对字节能被 31 整除
    if (data.empty()) {
        // 空输入也要一个 BFINAL=1、LEN=0 的块，否则 inflate 会等不到结尾。
        out.push_back(0x01);
        out.push_back(0);
        out.push_back(0);
        out.push_back(0xff);
        out.push_back(0xff);
    }
    std::size_t off = 0;
    while (off < data.size()) {
        const std::size_t n = std::min(kMaxStoredBlock, data.size() - off);
        const bool final = off + n >= data.size();
        out.push_back(final ? 0x01 : 0x00); // BFINAL + BTYPE=00，其余位补零
        out.push_back(static_cast<uint8_t>(n & 0xFF));
        out.push_back(static_cast<uint8_t>(n >> 8));
        const uint16_t nlen = static_cast<uint16_t>(~n);
        out.push_back(static_cast<uint8_t>(nlen & 0xFF));
        out.push_back(static_cast<uint8_t>(nlen >> 8));
        out.insert(out.end(), data.begin() + static_cast<std::ptrdiff_t>(off), data.begin() + static_cast<std::ptrdiff_t>(off + n));
        off += n;
    }
    const uint32_t sum = adler32(data.data(), data.size());
    out.push_back(static_cast<uint8_t>(sum >> 24));
    out.push_back(static_cast<uint8_t>(sum >> 16));
    out.push_back(static_cast<uint8_t>(sum >> 8));
    out.push_back(static_cast<uint8_t>(sum));
    return out;
}

} // namespace maa::ios::util
