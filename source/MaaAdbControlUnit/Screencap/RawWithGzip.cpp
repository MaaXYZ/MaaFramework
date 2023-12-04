#include "RawWithGzip.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapRawWithGzip::parse(const json::value& config)
{
    static const json::array kDefaultScreencapRawWithGzipArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "exec-out", "screencap | gzip -1",
    };

    return parse_argv("ScreencapRawWithGzip", config, kDefaultScreencapRawWithGzipArgv, screencap_raw_with_gzip_argv_);
}

bool ScreencapRawWithGzip::init(int swidth, int sheight)
{
    return set_wh(swidth, sheight);
}

std::optional<cv::Mat> ScreencapRawWithGzip::screencap()
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto output_opt = command(screencap_raw_with_gzip_argv_.gen(argv_replace_));

    if (!output_opt) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        output_opt.value(), std::bind(&ScreencapHelper::decode_gzip, &screencap_helper_, std::placeholders::_1));
}

MAA_CTRL_UNIT_NS_END