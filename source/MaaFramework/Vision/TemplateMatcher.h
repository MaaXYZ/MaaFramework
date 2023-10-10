#pragma once

#include <ostream>
#include <vector>

#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class TemplateMatcher : public VisionBase
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
    void set_templates(std::vector<std::shared_ptr<cv::Mat>> templates) { templates_ = std::move(templates); }
    void set_param(TemplateMatcherParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois(const cv::Mat& templ) const;
    ResultsVec match_and_postproc(const cv::Rect& roi, const cv::Mat& templ) const;
    void draw_result(const cv::Rect& roi, const cv::Mat& templ, const ResultsVec& results) const;

    void filter(ResultsVec& results, double threshold) const;

    TemplateMatcherParam param_;
    std::vector<std::shared_ptr<cv::Mat>> templates_;
};

MAA_VISION_NS_END

MAA_NS_BEGIN

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::TemplateMatcher::Result& res)
{
    os << res.to_json().to_string();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::TemplateMatcher::ResultsVec& resutls)
{
    json::array root;
    for (const auto& res : resutls) {
        root.emplace_back(res.to_json());
    }
    os << root.to_string();
    return os;
}

MAA_NS_END
