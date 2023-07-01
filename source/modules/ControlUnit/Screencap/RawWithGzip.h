#pragma once

#include "UnitBase.h"

#include "ScreencapHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawWithGzip : public ScreencapBase
{
public:
    virtual ~ScreencapRawWithGzip() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int w, int h) override;
    virtual void deinit() override {}

    virtual std::optional<cv::Mat> screencap() override;

private:
    ScreencapHelper screencap_helper_;

    Argv screencap_raw_with_gzip_argv_;
};

MAA_CTRL_UNIT_NS_END
