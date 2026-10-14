#include "decode/Decoder.h"

namespace maa::ios
{

/// 非 Apple 构建的默认后端直接使用软件工厂，当前不在此选择硬件加速器。
/// 未编入 libavcodec 时返回 nullptr，调用方需检查结果。
std::unique_ptr<Decoder> create_platform_decoder()
{
    return create_software_decoder();
}

} // namespace maa::ios
