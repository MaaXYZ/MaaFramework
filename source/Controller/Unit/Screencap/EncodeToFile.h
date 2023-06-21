#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapEncodeToFileAndPull : public ScreencapBase
{
public:
    bool parse(const json::value& config) override;

    bool init(int w, int h) override;

    std::optional<cv::Mat> screencap() override;

private:
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
