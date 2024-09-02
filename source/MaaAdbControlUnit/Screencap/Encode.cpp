#include "Encode.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncode::parse(const json::value& config)
{
    static const json::array kDefaultScreencapEncodeArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "exec-out", "screencap -p",
    };

    return parse_command("ScreencapEncode", config, kDefaultScreencapEncodeArgv, screencap_encode_argv_);
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

    return screencap_helper_.process_data(*output_opt, ScreencapHelper::decode_png);
}

MAA_CTRL_UNIT_NS_END
