#pragma once

#include "Base/UnitBase.h"

#include "ScreencapHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawWithGzip : public ScreencapBase
{
public:
    virtual ~ScreencapRawWithGzip() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override { return true; }

    virtual std::optional<cv::Mat> screencap() override;

private:
    ProcessArgvGenerator screencap_raw_with_gzip_argv_;
};

MAA_CTRL_UNIT_NS_END
