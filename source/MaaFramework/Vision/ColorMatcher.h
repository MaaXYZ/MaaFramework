#pragma once

#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class ColorMatcher : public VisionBase
{
public:
    struct Result
    {
        cv::Rect box {};
        int score = 0;

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["count"] = score;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    void set_param(ColorMatcherParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois(const ColorMatcherParam::Range& range, bool connected) const;
    ResultsVec color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range, bool connected) const;
    ResultsVec count_non_zero(const cv::Mat& bin, const cv::Point& tl) const;
    ResultsVec count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const;
    void draw_result(const cv::Rect& roi, const cv::Mat& color, const cv::Mat& bin, const ResultsVec& results) const;

    void filter(ResultsVec& results, int count) const;

    ColorMatcherParam param_;
};

MAA_VISION_NS_END

MAA_NS_BEGIN

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::ColorMatcher::Result& res)
{
    os << res.to_json().to_string();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::ColorMatcher::ResultsVec& resutls)
{
    json::array root;
    for (const auto& res : resutls) {
        root.emplace_back(res.to_json());
    }
    os << root.to_string();
    return os;
}

MAA_NS_END
