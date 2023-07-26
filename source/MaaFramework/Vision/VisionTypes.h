#pragma once

#include "MaaConf.h"

#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Utils/NoWarningCVMat.h"

MAA_VISION_NS_BEGIN

struct DirectHitParam
{
    std::vector<cv::Rect> roi; // only for "ClickSelf"
};

struct TemplMatchingParam
{
    inline static constexpr double kDefaultThreshold = 0.7;
    inline static constexpr int kDefaultMethod = 5; // cv::TM_CCOEFF_NORMED

    std::vector<cv::Rect> roi;
    std::vector<std::string> template_paths;
    std::vector<cv::Mat> template_images;
    std::vector<double> thresholds;
    int method = kDefaultMethod;
    bool green_mask = false;
};

struct OcrParam
{
    bool only_rec = false;
    std::vector<cv::Rect> roi;
    std::vector<std::string> text;
    std::vector<std::pair<std::string, std::string>> replace;
};

struct CompParam
{
    std::vector<cv::Rect> roi;
    double threshold = 0.0;
    int method = 0;
};

struct CustomParam
{
    std::string name;
    json::value custom_param;
};

MAA_VISION_NS_END
