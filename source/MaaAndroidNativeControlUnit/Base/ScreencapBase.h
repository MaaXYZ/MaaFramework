#pragma once

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapBase
{
public:
    virtual ~ScreencapBase() = default;

    virtual bool init() = 0;
    virtual bool screencap(cv::Mat& image) = 0;
    virtual std::pair<int, int> get_resolution() const = 0;
};

MAA_CTRL_UNIT_NS_END
