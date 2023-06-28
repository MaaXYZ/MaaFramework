#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <string>
#include <unordered_map>
#include <vector>

MAA_VISION_NS_BEGIN

struct TemplMatchingParams
{
    std::vector<std::string> templates;
    std::vector<double> thresholds;
    int method = 0;
    bool green_mask = false;
};

struct OcrParams
{
    std::vector<std::string> text;
    std::vector<std::pair<std::string, std::string>> replace;
};

struct FreezesWaitingParams
{
    double threshold = 0.0;
    int method = 0;
    uint wait_time = 0;
};

MAA_VISION_NS_END
