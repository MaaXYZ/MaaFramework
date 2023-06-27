#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI_DEBUG ScreencapRawWithGzip : public ScreencapBase
{
public:
    virtual ~ScreencapRawWithGzip() override = default;

    virtual bool parse(const json::value& config) override;

    virtual bool init(int w, int h) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_raw_with_gzip_argv_;
};

MAA_CTRL_UNIT_NS_END
