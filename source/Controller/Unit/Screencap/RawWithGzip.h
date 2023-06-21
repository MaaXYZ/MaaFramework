#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawWithGzip : public ScreencapBase
{
public:
    bool parse(const json::value& config) override;

    bool init(int w, int h) override;

    std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_raw_with_gzip_argv_;
};

MAA_CTRL_UNIT_NS_END
