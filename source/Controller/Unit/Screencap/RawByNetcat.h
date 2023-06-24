#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawByNetcat : public ScreencapBase
{
public:
    virtual ~ScreencapRawByNetcat() override = default;

    virtual bool parse(const json::value& config) override;

    virtual bool init(int w, int h) override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    std::optional<std::string> request_netcat_address();

    Argv screencap_raw_by_netcat_argv_;
    Argv netcat_address_argv_;

    std::string netcat_address_;
    uint16_t netcat_port_ = 0;
};

MAA_CTRL_UNIT_NS_END
