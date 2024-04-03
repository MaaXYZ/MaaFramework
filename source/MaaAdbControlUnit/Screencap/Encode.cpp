#include "Encode.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncode::parse(const json::value& config)
{
    static const json::array kDefaultScreencapEncodeArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "exec-out", "screencap -p",
    };

    return parse_argv(
        "ScreencapEncode",
        config,
        kDefaultScreencapEncodeArgv,
        screencap_encode_argv_);
}

bool ScreencapEncode::init(int swidth, int sheight)
{
    return set_wh(swidth, sheight);
}

std::optional<cv::Mat> ScreencapEncode::screencap()
{
    auto argv_opt = screencap_encode_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        *output_opt,
        std::bind(&ScreencapHelper::decode_png, &screencap_helper_, std::placeholders::_1));
}

MAA_CTRL_UNIT_NS_END