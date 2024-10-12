#pragma once

#include "Base/UnitBase.h"

#include "ScreencapHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapEncodeToFileAndPull : public ScreencapBase
{
public:
    virtual ~ScreencapEncodeToFileAndPull() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    ProcessArgvGenerator screencap_encode_to_file_argv_;
    ProcessArgvGenerator pull_file_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
