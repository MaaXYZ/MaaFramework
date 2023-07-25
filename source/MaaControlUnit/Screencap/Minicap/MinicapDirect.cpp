#include "MinicapDirect.h"

#include "Utils/Logger.hpp"
#include "Utils/Format.hpp"
#include "Utils/NoWarningCV.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> MinicapDirect::screencap()
{
    int width = screencap_helper_.get_w();
    int height = screencap_helper_.get_h();

    auto res = binary_->invoke_bin_stdout(MAA_FMT::format("-P {}x{}@{}x{}/{} -s", width, height, width, height, 0));

    if (!res) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(res.value(), std::bind(&ScreencapHelper::decode_jpg_with_minicap_header,
                                                                 &screencap_helper_, std::placeholders::_1));
}
MAA_CTRL_UNIT_NS_END
