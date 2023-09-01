#pragma once

#include "Conf/Conf.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Utils/NoWarningCVMat.hpp"

namespace Ort
{
class Session;
}

MAA_VISION_NS_BEGIN

struct DirectHitParam
{};

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

struct ClassifierParam
{
    size_t cls_size = 0;
    std::vector<std::string> labels; // only for output and debug
    std::string model_path;
    std::shared_ptr<Ort::Session> session = nullptr;

    std::vector<cv::Rect> roi;
    std::vector</*index*/ size_t> expected;
};

struct DetectorParam
{
    enum class Net
    {
        YoloV8,
    };
    inline static constexpr Net kDefaultNet = Net::YoloV8;

    size_t cls_size = 0;
    std::vector<std::string> labels; // only for output and debug
    std::string model_path;
    std::shared_ptr<Ort::Session> session = nullptr;
    Net net = kDefaultNet;

    std::vector<cv::Rect> roi;
    std::vector</*index*/ size_t> expected;
    std::vector<double> thresholds;
};

MAA_VISION_NS_END
