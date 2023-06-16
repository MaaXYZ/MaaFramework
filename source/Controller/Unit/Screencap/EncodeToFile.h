#pragma once

#include "ScreencapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapEncodeToFileAndPull : public ScreencapBase
{
public:
    bool parse(const json::value& config);

    bool init(int w, int h, const std::string& force_temp = "");
    void deinit() {}

    std::optional<cv::Mat> screencap();

#ifdef MAA_DEBUG
    const std::string& get_tempname() const { return tempname_; }
#endif

private:
    Argv screencap_encode_to_file_argv_;
    Argv pull_file_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
