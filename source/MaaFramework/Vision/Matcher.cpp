#include "Matcher.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Matcher::ResultOpt Matcher::analyze() const
{
    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }
    if (param_.template_images.empty()) {
        LogError << name_ << "templates is empty" << VAR(param_.template_paths);
        return std::nullopt;
    }

    if (param_.template_images.size() != param_.thresholds.size()) {
        LogError << name_ << "templates.size() != thresholds.size()" << VAR(param_.template_images.size())
                 << VAR(param_.thresholds.size());
        return std::nullopt;
    }

    for (size_t i = 0; i != param_.template_images.size(); ++i) {
        const cv::Mat& templ = param_.template_images.at(i);
        if (templ.empty()) {
            LogWarn << name_ << "template is empty" << VAR(param_.template_paths) << VAR(i) << VAR(templ);
            continue;
        }
        double threshold = param_.thresholds.at(i);
        auto start = std::chrono::steady_clock::now();

        auto res = traverse_rois(templ, threshold);

        auto costs = duration_since(start);
        LogDebug << name_ << param_.template_paths.at(i) << VAR(res.score) << VAR(threshold) << VAR(costs);

        if (res.score > threshold) {
            return res;
        }
    }

    return std::nullopt;
}

Matcher::Result Matcher::traverse_rois(const cv::Mat& templ, double threshold) const
{
    if (!cache_.empty()) {
        return match_and_postproc(cache_, templ);
    }

    if (param_.roi.empty()) {
        return match_and_postproc(cv::Rect(0, 0, image_.cols, image_.rows), templ);
    }

    Result res;
    for (const cv::Rect& roi : param_.roi) {
        res = match_and_postproc(roi, templ);
        if (res.score > threshold) {
            break;
        }
    }

    return res;
}

Matcher::Result Matcher::match_and_postproc(const cv::Rect& roi, const cv::Mat& templ) const
{
    cv::Mat image = image_with_roi(roi);
    cv::Mat matched = match_template(image, templ, param_.method, param_.green_mask);
    if (matched.empty()) {
        return {};
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (std::isnan(max_val) || std::isinf(max_val)) {
        max_val = 0;
    }

    cv::Rect box(max_loc.x + roi.x, max_loc.y + roi.y, templ.cols, templ.rows);

    cv::Mat image_draw;
    if (debug_draw_) {
        int raw_width = image_.cols;
        image_draw = draw_roi(roi);

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, box, color, 1);
        std::string flag =
            MAA_FMT::format("Res: {:.3f}, [{}, {}, {}, {}]", max_val, box.x, box.y, box.width, box.height);
        cv::putText(image_draw, flag, cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);

        cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, templ.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        cv::Mat draw_templ_roi = image_draw(cv::Rect(raw_width, 0, templ.cols, templ.rows));
        templ.copyTo(draw_templ_roi);
        cv::line(image_draw, cv::Point(raw_width, 0), cv::Point(box.x, box.y), color, 1);
    }

    if (save_draw_) {
        save_image(image_draw);
    }

    return Result { .box = box, .score = max_val };
}

MAA_VISION_NS_END
