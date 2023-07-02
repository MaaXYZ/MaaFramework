#include "Matcher.h"

#include "Resource/ResourceMgr.h"
#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/StringMisc.hpp"

MAA_VISION_NS_BEGIN

Matcher::ResultOpt Matcher::analyze() const
{
    if (param_.templates.size() != param_.thresholds.size()) {
        LogError << "templates.size() != thresholds.size()" << VAR(param_.templates.size())
                 << VAR(param_.thresholds.size());
        return std::nullopt;
    }

    for (size_t i = 0; i != param_.templates.size(); ++i) {
        const std::string& name = param_.templates.at(i);
        const cv::Mat& templ = resource()->template_cfg().get_template_image(name);
        if (templ.empty()) {
            LogWarn << "template is empty" << VAR(name) << VAR(i) << VAR(templ);
            continue;
        }

        auto opt = match_and_postproc(templ, param_.thresholds.at(i));
        if (opt) {
            return opt;
        }
    }

    return std::nullopt;
}

Matcher::ResultOpt Matcher::match_and_postproc(const cv::Mat& templ, double threshold) const
{
    cv::Mat matched = match_template(image_(roi_), templ, param_.method, param_.green_mask);
    if (matched.empty()) {
        return std::nullopt;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (std::isnan(max_val) || std::isinf(max_val)) {
        max_val = 0;
    }
    if (max_val < threshold) {
        return std::nullopt;
    }

    cv::Rect box(max_loc.x + roi_.x, max_loc.y + roi_.y, templ.cols, templ.rows);
    return Result { .box = box, .score = max_val };
}

cv::Mat Matcher::match_template(const cv::Mat& image, const cv::Mat& templ, int method, bool green_mask)
{
    if (templ.cols > image.cols || templ.rows > image.rows) {
        LogError << "templ size is too large" << VAR(image) << VAR(templ);
        return {};
    }

    auto mask = cv::noArray();
    if (green_mask) {
        cv::inRange(templ, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
    }

    cv::Mat matched;
    cv::matchTemplate(image, templ, matched, method, mask);
    return matched;
}

MAA_VISION_NS_END
