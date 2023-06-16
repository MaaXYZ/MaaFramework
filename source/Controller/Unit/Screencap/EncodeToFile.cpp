#include "EncodeToFile.h"

#include "../utils.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncodeToFileAndPull::parse(const json::value& config)
{
    return parse_argv("ScreencapEncodeToFile", config, screencap_encode_to_file_argv_) &&
           parse_argv("PullFile", config, pull_file_argv_);
}

bool ScreencapEncodeToFileAndPull::init(int w, int h, const std::string& force_temp)
{
    set_wh(w, h);

    tempname_ = force_temp.empty() ? temp_name() : force_temp;

    return true;
}

std::optional<cv::Mat> ScreencapEncodeToFileAndPull::screencap()
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto dst_path = temp_path(tempname_);

    merge_replacement({ { "{TEMP_FILE}", tempname_ }, { "{DST_PATH}", path_to_crt_string(dst_path) } });
    auto cmd_ret = command(screencap_encode_to_file_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    cmd_ret = command(pull_file_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    return cv::imread(dst_path);
}

MAA_CTRL_UNIT_NS_END
