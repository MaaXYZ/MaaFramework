#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAA_DEBUG_API ScreencapEncodeToFileAndPull : public ScreencapBase
{
public:
    virtual ~ScreencapEncodeToFileAndPull() override = default;

    virtual bool parse(const json::value& config) override;

    virtual bool init(int w, int h) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
