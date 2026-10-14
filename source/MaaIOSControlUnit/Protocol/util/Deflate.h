#pragma once

#include <cstdint>
#include <vector>

namespace maa::ios::util
{

/// 把一段数据封成 zlib 流，**不压缩**（deflate "stored" 块）。
///
/// 为什么自己写而不链 zlib：这里要的不是压缩率，而是"对方 inflate 得开"。
/// CoreDevice 的媒体协商 blob 只有两三百字节，压不压对带宽毫无影响，而 stored
/// 块的编码就是「原样拷贝 + 长度取反」，二十来行、没有状态机、错了也一眼能看出来。
/// 少一个跨三个平台都要构建的依赖，值这个交换。
///
/// 产出：2 字节 zlib 头（0x78 0x9c，CM=8 且校验位满足 (CMF*256+FLG) % 31 == 0）
/// + 若干 stored 块（BFINAL=1 落在最后一块上）+ 4 字节 Adler-32。
/// 单块载荷上限 65535 字节，超了就分块。
[[nodiscard]] std::vector<uint8_t> zlib_store(std::vector<uint8_t> data);

[[nodiscard]] uint32_t adler32(const uint8_t* data, std::size_t len);

} // namespace maa::ios::util
