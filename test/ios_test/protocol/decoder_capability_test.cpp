#include "decode/Decoder.h"

#include <cstdio>

#ifdef MAA_IOS_HAVE_LIBAV
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
}
#endif

int main()
{
#ifdef MAA_IOS_HAVE_LIBAV
    const bool hevc = avcodec_find_decoder(AV_CODEC_ID_HEVC) != nullptr;
    const bool png = avcodec_find_decoder(AV_CODEC_ID_PNG) != nullptr;
    if (!hevc) {
        std::fputs("MaaIOS requires an FFmpeg HEVC video decoder\n", stderr);
    }
    if (!png) {
        std::fputs("MaaIOS requires an FFmpeg PNG screenshot decoder\n", stderr);
    }
    const int original_log_level = av_log_get_level();
    av_log_set_level(AV_LOG_WARNING);
    auto decoder = maa::ios::create_software_decoder();
    const bool log_unchanged = av_log_get_level() == AV_LOG_WARNING;
    av_log_set_level(original_log_level);
    if (!decoder || !log_unchanged) {
        std::fputs("Creating an iOS decoder must preserve the host FFmpeg log level\n", stderr);
    }
    return hevc && png && decoder && log_unchanged ? 0 : 1;
#else
    if (maa::ios::create_software_decoder()) {
        std::fputs("FFmpeg-disabled build unexpectedly has a software decoder\n", stderr);
        return 1;
    }
#ifdef __APPLE__
    return maa::ios::kHaveDecoder && maa::ios::create_platform_decoder() ? 0 : 1;
#else
    return !maa::ios::kHaveDecoder && !maa::ios::create_platform_decoder() ? 0 : 1;
#endif
#endif
}
