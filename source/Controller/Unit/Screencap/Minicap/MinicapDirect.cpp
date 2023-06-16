#include "MinicapDirect.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

#include <format>

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> MinicapDirect::screencap()
{
    auto res = binary_->invoke_bin_stdout(std::format("-P {}x{}@{}x{}/{} -s", width_, height_, width_, height_, 0));

    if (!res) {
        return std::nullopt;
    }

    return process_data(res.value(),
                        std::bind(&ScreencapBase::decode_jpg_with_minicap_header, this, std::placeholders::_1));
}
MAA_CTRL_UNIT_NS_END
