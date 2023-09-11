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
        int count = 0;
        cv::Mat dst {};

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["count"] = count;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    void set_param(ColorMatcherParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois(const ColorMatcherParam::Range& range, bool connected) const;
    Result color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range, bool connected) const;
    void draw_result(const cv::Rect& roi, const cv::Mat& color, const Result& res) const;

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
