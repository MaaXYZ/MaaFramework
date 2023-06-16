#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawWithGzip : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h);
    void deinit() {}

    std::optional<cv::Mat> screencap();

private:
    Argv screencap_raw_with_gzip_argv_;
};

MAA_CTRL_UNIT_NS_END
