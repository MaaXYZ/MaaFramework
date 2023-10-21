#pragma once

#include "UnitBase.h"

#include "ScreencapHelper.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class ScreencapEncode : public ScreencapBase
{
public:
    virtual ~ScreencapEncode() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;
    virtual void deinit() override {}

    virtual std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_encode_argv_;
};

MAA_ADB_CTRL_UNIT_NS_END
