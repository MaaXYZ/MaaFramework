#include "MinicapDirect.h"

#include <format>

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapDirect::init()
{
    return init_binary();
}

void MinicapDirect::on_display_changed(int width, int height)
{
    LogDebug << VAR(width) << VAR(height);

    display_width_ = width;
    display_height_ = height;
}

std::optional<cv::Mat> MinicapDirect::screencap()
{
    auto res = binary_->invoke_bin_and_read_pipe(std::format(
        "-P {}x{}@{}x{}/{} -s",
        display_width_,
        display_height_,
        display_width_,
        display_height_,
        0));

    if (!res) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(*res, ScreencapHelper::trunc_decode_jpg);
}

MAA_CTRL_UNIT_NS_END
