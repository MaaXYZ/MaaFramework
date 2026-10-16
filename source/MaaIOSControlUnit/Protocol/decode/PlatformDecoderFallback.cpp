#include "decode/Decoder.h"

namespace maa::ios
{

/// 非 Apple 构建的默认后端直接使用软件工厂，当前不在此选择硬件加速器。
/// 未编入 libavcodec 时返回 nullptr，调用方需检查结果。
std::unique_ptr<Decoder> create_platform_decoder()
{
    return create_software_decoder();
}

#ifndef MAA_IOS_HAVE_LIBAV
/// 软件后端未编入时它的实现恒返回 nullptr，调用方据此退回平台后端或报错退出。
std::unique_ptr<Decoder> create_software_decoder()
{
    return nullptr;
}
#endif

} // namespace maa::ios
