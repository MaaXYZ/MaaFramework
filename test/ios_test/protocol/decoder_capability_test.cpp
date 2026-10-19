#include "decode/Decoder.h"

#include <cstdio>

#ifdef MAA_IOS_HAVE_LIBAV
extern "C"
{
#include <libavcodec/avcodec.h>
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
    return hevc && png ? 0 : 1;
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
