#pragma once

#include "Base/UnitBase.h"

#include "ScreencapHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapRawByNetcat : public ScreencapBase
{
public:
    virtual ~ScreencapRawByNetcat() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapAPI
    virtual bool init(int swidth, int sheight) override;
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
