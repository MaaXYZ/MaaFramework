#include "MinicapDirect.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> MinicapDirect::screencap()
{
    int width = screencap_helper_.get_w();
    int height = screencap_helper_.get_h();

    auto res = binary_->invoke_bin_stdout(MAA_FMT::format("-P {}x{}@{}x{}/{} -s", width, height, width, height, 0));

    if (!res) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        res.value(), std::bind(&ScreencapHelper::trunc_decode_jpg, &screencap_helper_, std::placeholders::_1));
}
MAA_CTRL_UNIT_NS_END
