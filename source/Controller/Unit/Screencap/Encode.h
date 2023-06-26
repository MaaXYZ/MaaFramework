#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI ScreencapEncode : public ScreencapBase
{
public:
    virtual ~ScreencapEncode() override = default;

    virtual bool parse(const json::value& config) override;

    virtual bool init(int w, int h) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_encode_argv_;
};

MAA_CTRL_UNIT_NS_END
