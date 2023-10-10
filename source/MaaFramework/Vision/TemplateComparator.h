#pragma once

#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class TemplateComparator
{
public:
    struct Result
    {
        cv::Rect box {};
        double score = 0.0;

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["score"] = score;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    TemplateComparator() = default;

    void set_param(TemplateComparatorParam param) { param_ = std::move(param); }
    ResultsVec analyze(const cv::Mat& lhs, const cv::Mat& rhs) const;

private:
    ResultsVec foreach_rois(const cv::Mat& lhs, const cv::Mat& rhs) const;
    void filter(ResultsVec& results, double threshold) const;

    static double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);

    TemplateComparatorParam param_;
};

MAA_VISION_NS_END

MAA_NS_BEGIN

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::TemplateComparator::Result& res)
{
    os << res.to_json().to_string();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::TemplateComparator::ResultsVec& resutls)
{
    json::array root;
    for (const auto& res : resutls) {
        root.emplace_back(res.to_json());
    }
    os << root.to_string();
    return os;
}

MAA_NS_END
