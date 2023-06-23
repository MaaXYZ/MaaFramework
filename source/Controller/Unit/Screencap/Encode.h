#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapEncode : public ScreencapBase
{
public:
    bool parse(const json::value& config) override;

    bool init(int w, int h) override;

    std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_encode_argv_;
};

MAA_CTRL_UNIT_NS_END
