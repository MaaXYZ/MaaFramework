#include "EncodeToFile.h"

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/Time.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncodeToFileAndPull::parse(const json::value& config)
{
    static const json::array kDefaultScreencapEncodeToFileArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "screencap -p > \"/data/local/tmp/{TEMP_FILE}\"",
    };
    static const json::array kDefaultPullFileArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "pull", "/data/local/tmp/{TEMP_FILE}", "{DST_PATH}"
    };

    return parse_argv("ScreencapEncodeToFile", config, kDefaultScreencapEncodeToFileArgv,
                      screencap_encode_to_file_argv_) &&
           parse_argv("PullFile", config, kDefaultPullFileArgv, pull_file_argv_);
}

bool ScreencapEncodeToFileAndPull::init(int swidth, int sheight)
{
    tempname_ = now_filestem();
    return set_wh(swidth, sheight);
}

std::optional<cv::Mat> ScreencapEncodeToFileAndPull::screencap()
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto dst_path = std::filesystem::temp_directory_path() / now_filestem();

    merge_replacement({ { "{TEMP_FILE}", tempname_ }, { "{DST_PATH}", path_to_crt_string(dst_path) } });
    auto output_opt = command(screencap_encode_to_file_argv_.gen(argv_replace_));

    if (!output_opt) {
        return std::nullopt;
    }

    output_opt = command(pull_file_argv_.gen(argv_replace_));

    if (!output_opt) {
        return std::nullopt;
    }

    return imread(dst_path);
}

MAA_CTRL_UNIT_NS_END