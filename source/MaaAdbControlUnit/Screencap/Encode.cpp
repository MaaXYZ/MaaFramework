#include "Encode.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncode::parse(const json::value& config)
{
    return parse_argv("ScreencapEncode", config, screencap_encode_argv_);
}

bool ScreencapEncode::init(int swidth, int sheight)
{
    return set_wh(swidth, sheight);
}

std::optional<cv::Mat> ScreencapEncode::screencap()
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto cmd_ret = command(screencap_encode_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        cmd_ret.value(), std::bind(&ScreencapHelper::decode_png, &screencap_helper_, std::placeholders::_1));
}

MAA_CTRL_UNIT_NS_END
