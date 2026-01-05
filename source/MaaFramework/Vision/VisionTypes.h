#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

enum class TargetType
{
    Invalid = 0,
    Self,
    PreTask,
    Region,
};

struct TargetObj
{
    using Type = TargetType;

    Type type = Type::Self;
    std::variant<std::monostate, std::string, cv::Rect> param;
};

struct Target : public TargetObj
{
    using Type = TargetType;

    Type type = Type::Self;
    std::variant<std::monostate, std::string, cv::Rect> param;
    cv::Rect offset {};
};

enum class ResultOrderBy
{
    Horizontal,
    Vertical,
    Score,
    Area,
    Length, // for OCR
    Random,
    Expected,
};

struct RoiTargetParamBase
{
    Target roi_target;
};

struct DirectHitParam : public RoiTargetParamBase
{
};

struct TemplateMatcherParam : public RoiTargetParamBase
{
    inline static constexpr double kDefaultThreshold = 0.7;
    inline static constexpr int kDefaultMethod = 5; // cv::TM_CCOEFF_NORMED
    inline static constexpr int kMethodInvertBase = 10000;

    std::vector<std::string> template_;
    std::vector<double> thresholds = { kDefaultThreshold };
    int method = kDefaultMethod;
    bool green_mask = false;

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct OCRerParam : public RoiTargetParamBase
{
    inline static constexpr double kDefaultThreshold = 0.3;

    std::string model;
    bool only_rec = false;
    std::vector<std::wstring> expected;
    double threshold = kDefaultThreshold;
    std::vector<std::pair<std::wstring, std::wstring>> replace;

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct TemplateComparatorParam : public RoiTargetParamBase
{
    double threshold = 0.0;
    int method = 0;
};

struct CustomRecognitionParam : public RoiTargetParamBase
{
    std::string name;
    json::value custom_param;
};

struct NeuralNetworkClassifierParam : public RoiTargetParamBase
{
    std::string model;
    std::vector<std::string> labels; // only for output and debug
    std::vector</*result_index*/ int> expected;

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct NeuralNetworkDetectorParam : public RoiTargetParamBase
{
    enum class Net
    {
        YoloV8,
    };
    inline static constexpr Net kDefaultNet = Net::YoloV8;
    inline static constexpr double kDefaultThreshold = 0.3;

    std::string model;
    Net net = kDefaultNet;
    std::vector<std::string> labels; // only for output and debug
    std::vector</*result_index*/ int> expected;
    std::vector<double> thresholds = { kDefaultThreshold };

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct ColorMatcherParam : public RoiTargetParamBase
{
    inline static constexpr int kDefaultCount = 1;
    inline static constexpr int kDefaultMethod = 4; // cv::COLOR_BGR2RGB
    using Range = std::pair<std::vector<int>, std::vector<int>>;

    std::vector<Range> range;
    int count = kDefaultCount;
    int method = kDefaultMethod;
    bool connected = false; // 是否计算连通域

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct FeatureMatcherParam : public RoiTargetParamBase
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
    inline static constexpr double kDefaultkDefaultRatio = 0.6;
    inline static constexpr int kDefaultCount = 4;

    std::vector<std::string> template_;
    bool green_mask = false;

    Detector detector = kDefaultDetector;
    // Matcher matcher = kDefaultMatcher;

    double ratio = kDefaultkDefaultRatio;
    int count = kDefaultCount;

    ResultOrderBy order_by = ResultOrderBy::Horizontal;
    int result_index = 0;
};

struct RectComparator
{
    bool operator()(const cv::Rect& lhs, const cv::Rect& rhs) const
    {
        if (lhs.x != rhs.x) {
            return lhs.x < rhs.x;
        }
        if (lhs.y != rhs.y) {
            return lhs.y < rhs.y;
        }
        if (lhs.width != rhs.width) {
            return lhs.width < rhs.width;
        }
        if (lhs.height != rhs.height) {
            return lhs.height < rhs.height;
        }
        return false;
    }
};

MAA_VISION_NS_END
