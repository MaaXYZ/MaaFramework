#include "MinicapDirect.h"

#include <format>

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

MinicapDirect::~MinicapDirect()
{
    deinit_binary();
}

bool MinicapDirect::init()
{
    return init_binary();
}

std::optional<cv::Mat> MinicapDirect::screencap()
{
    auto res = binary_->invoke_bin_and_read_pipe(
        std::format("-P {}x{}@{}x{}/{} -s", display_width_, display_height_, display_width_, display_height_, 0));

    if (!res) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(*res, ScreencapHelper::trunc_decode_jpg);
}

MAA_CTRL_UNIT_NS_END
