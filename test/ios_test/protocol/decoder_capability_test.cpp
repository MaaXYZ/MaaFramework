#include <cstdio>
extern "C"
{
#include <libavcodec/avcodec.h>
}

int main()
{
    const bool hevc = avcodec_find_decoder(AV_CODEC_ID_HEVC) != nullptr;
    const bool png = avcodec_find_decoder(AV_CODEC_ID_PNG) != nullptr;
    if (!hevc) {
        std::fputs("MaaIOS requires an FFmpeg HEVC video decoder\n", stderr);
    }
    if (!png) {
        std::fputs("MaaIOS requires an FFmpeg PNG screenshot decoder\n", stderr);
    }
    return hevc && png ? 0 : 1;
}
