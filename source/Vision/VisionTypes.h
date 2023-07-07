#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <string>
#include <unordered_map>
#include <vector>

MAA_VISION_NS_BEGIN

struct DirectHitParams
{
    std::vector<cv::Rect> roi; // only for "ClickSelf"
};

struct TemplMatchingParams
{
    std::vector<cv::Rect> roi;
    std::vector<std::string> template_paths;
    std::vector<cv::Mat> template_images;
    std::vector<double> thresholds;
    int method = 0;
    bool green_mask = false;
};

struct OcrParams
{
    bool only_rec = false;
    std::vector<cv::Rect> roi;
    std::vector<std::string> text;
    std::vector<std::pair<std::string, std::string>> replace;
};

struct CompParams
{
    std::vector<cv::Rect> roi;
    double threshold = 0.0;
    int method = 0;
};

MAA_VISION_NS_END
