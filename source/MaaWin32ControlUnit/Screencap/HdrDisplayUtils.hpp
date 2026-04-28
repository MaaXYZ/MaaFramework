#pragma once

#include <optional>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

struct HdrDisplayState
{
    bool hdr_enabled = false;
    bool hdr_user_enabled = false;
    int active_color_mode = 0;
    float sdr_white_nits = 80.0f;
    bool valid = false;
};

std::optional<bool> query_windows_hdr_enabled_go_compatible();
std::optional<HdrDisplayState> query_hdr_display_state(HMONITOR target_monitor);
cv::Mat compensate_hdr_sdr_capture(const cv::Mat& bgra, float sdr_white_nits);

MAA_CTRL_UNIT_NS_END
