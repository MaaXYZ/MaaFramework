#pragma once

#include "Conf/Conf.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Utils/NoWarningCVMat.hpp"

#if __has_include(<opencv2/xfeatures2d.hpp>)
#define MAA_VISION_HAS_XFEATURES2D
#endif

namespace Ort
{
struct Session;
}

MAA_VISION_NS_BEGIN

struct DirectHitParam
{};

struct TemplateMatcherParam
{
    inline static constexpr double kDefaultThreshold = 0.7;
    inline static constexpr int kDefaultMethod = 5; // cv::TM_CCOEFF_NORMED

    std::vector<cv::Rect> roi;
    std::vector<std::string> template_paths;
    std::vector<double> thresholds;
    int method = kDefaultMethod;
    bool green_mask = false;
};

struct OCRerParam
{
    std::string model;
    bool only_rec = false;
    std::vector<cv::Rect> roi;
    std::vector<std::wstring> text;
    std::vector<std::pair<std::wstring, std::wstring>> replace;
};

struct TemplateComparatorParam
{
    std::vector<cv::Rect> roi;
    double threshold = 0.0;
    int method = 0;
};

struct CustomRecognizerParam
{
    std::string name;
    json::value custom_param;
};

struct NeuralNetworkClassifierParam
{
    size_t cls_size = 0;
    std::vector<std::string> labels; // only for output and debug
    std::string model;

    std::vector<cv::Rect> roi;
    std::vector</*index*/ size_t> expected;
};

struct NeuralNetworkDetectorParam
{
    enum class Net
    {
        YoloV8,
    };
    inline static constexpr Net kDefaultNet = Net::YoloV8;
    inline static constexpr double kDefaultThreshold = 0.3;

    size_t cls_size = 0;
    std::vector<std::string> labels; // only for output and debug
    std::string model;
    Net net = kDefaultNet;

    std::vector<cv::Rect> roi;
    std::vector</*index*/ size_t> expected;
    std::vector<double> thresholds;
};

struct ColorMatcherParam
{
    inline static constexpr int kDefaultCount = 1;
    inline static constexpr int kDefaultMethod = 4; // cv::COLOR_BGR2RGB
    using Range = std::pair<std::vector<int>, std::vector<int>>;

    std::vector<cv::Rect> roi;
    std::vector<Range> range;
    int count = kDefaultCount;
    int method = kDefaultMethod;
    bool connected = false; // 是否计算连通域
};

struct FeatureMatcherParam
{
    enum class Detector
    {
        SIFT,
        SURF,
        ORB,
        BRISK,
        KAZE,
        AKAZE,
    };

    // enum class Matcher
    //{
    //     FLANN,
    //     BRUTEFORCE,
    // };

    inline static constexpr Detector kDefaultDetector = Detector::SIFT;
    // inline static constexpr Matcher kDefaultMatcher = Matcher::FLANN;
    inline static constexpr double kDefaultDistanceRatio = 0.6;
    inline static constexpr int kDefaultCount = 4;

    std::vector<cv::Rect> roi;
    std::string template_path;
    bool green_mask = false;

    Detector detector = kDefaultDetector;
    // Matcher matcher = kDefaultMatcher;

    double distance_ratio = kDefaultDistanceRatio;
    int count = kDefaultCount;
};

MAA_VISION_NS_END
