#include "EncodeToFile.h"

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Time.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool ScreencapEncodeToFileAndPull::parse(const json::value& config)
{
    static const json::array kDefaultScreencapEncodeToFileArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "screencap -p > \"/data/local/tmp/{TEMP_FILE}\"",
    };
    static const json::array kDefaultPullFileArgv = { "{ADB}", "-s", "{ADB_SERIAL}", "pull", "/data/local/tmp/{TEMP_FILE}", "{DST_PATH}" };

    return parse_command("ScreencapEncodeToFile", config, kDefaultScreencapEncodeToFileArgv, screencap_encode_to_file_argv_)
           && parse_command("PullFile", config, kDefaultPullFileArgv, pull_file_argv_);
}

bool ScreencapEncodeToFileAndPull::init()
{
    tempname_ = format_now_for_filename();
    return true;
}

std::optional<cv::Mat> ScreencapEncodeToFileAndPull::screencap()
{
    auto dst_path = std::filesystem::temp_directory_path() / format_now_for_filename();

    merge_replacement({ { "{TEMP_FILE}", tempname_ }, { "{DST_PATH}", path_to_utf8_string(dst_path) } });

    {
        auto argv_opt = screencap_encode_to_file_argv_.gen(argv_replace_);
        if (!argv_opt) {
            return std::nullopt;
        }

        auto output_opt = startup_and_read_pipe(*argv_opt);
        if (!output_opt) {
            return std::nullopt;
        }
    }

    {
        auto argv_opt = pull_file_argv_.gen(argv_replace_);
        if (!argv_opt) {
            return std::nullopt;
        }

        auto output_opt = startup_and_read_pipe(*argv_opt);
        if (!output_opt) {
            return std::nullopt;
        }
    }

    auto image = imread(dst_path);
    std::error_code ec;
    std::filesystem::remove(dst_path, ec);

    if (image.empty()) {
        LogError << "Failed to read image from" << dst_path;
        return std::nullopt;
    }

    return image;
}

MAA_CTRL_UNIT_NS_END
