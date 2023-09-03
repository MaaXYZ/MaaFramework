#include "Matcher.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Matcher::ResultsVec Matcher::analyze() const
{
    if (param_.template_images.empty()) {
        LogError << name_ << "templates is empty" << VAR(param_.template_paths);
        return {};
    }

    if (param_.template_images.size() != param_.thresholds.size()) {
        LogError << name_ << "templates.size() != thresholds.size()" << VAR(param_.template_images.size())
                 << VAR(param_.thresholds.size());
        return {};
    }

    ResultsVec all_results;
    for (size_t i = 0; i != param_.template_images.size(); ++i) {
        const cv::Mat& templ = param_.template_images.at(i);

        auto start_time = std::chrono::steady_clock::now();
        ResultsVec results = foreach_rois(templ);

        auto costs = duration_since(start_time);
        const std::string& path = param_.template_paths.at(i);
        LogDebug << name_ << "Raw:" << VAR(results) << VAR(path) << VAR(costs);

        double threshold = param_.thresholds.at(i);
        filter(results, threshold);

        costs = duration_since(start_time);
        LogDebug << name_ << "Filter:" << VAR(results) << VAR(path) << VAR(threshold) << VAR(costs);

        all_results.insert(all_results.end(), std::make_move_iterator(results.begin()),
                           std::make_move_iterator(results.end()));
    }

    return all_results;
}

Matcher::ResultsVec Matcher::foreach_rois(const cv::Mat& templ) const
{
    if (templ.empty()) {
        LogWarn << name_ << "template is empty" << VAR(param_.template_paths) << VAR(templ);
        return {};
    }

    if (!cache_.empty()) {
        return { match_and_postproc(cache_, templ) };
    }

    if (param_.roi.empty()) {
        return { match_and_postproc(cv::Rect(0, 0, image_.cols, image_.rows), templ) };
    }

    ResultsVec res;
    for (const cv::Rect& roi : param_.roi) {
        Result temp = match_and_postproc(roi, templ);
        res.emplace_back(std::move(temp));
    }

    return res;
}

Matcher::Result Matcher::match_and_postproc(const cv::Rect& roi, const cv::Mat& templ) const
{
    auto start = std::chrono::steady_clock::now();

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
    Result result { .box = box, .score = max_val };

    draw_result(roi, templ, result);
    return result;
}

void Matcher::draw_result(const cv::Rect& roi, const cv::Mat& templ, const Result& result) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);
    const auto color = cv::Scalar(0, 0, 255);
    cv::rectangle(image_draw, result.box, color, 1);

    std::string flag = MAA_FMT::format("Res: {:.3f}, [{}, {}, {}, {}]", result.score, result.box.x, result.box.y,
                                       result.box.width, result.box.height);
    cv::putText(image_draw, flag, cv::Point(result.box.x, result.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);

    int raw_width = image_.cols;
    cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, templ.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::Mat draw_templ_roi = image_draw(cv::Rect(raw_width, 0, templ.cols, templ.rows));
    templ.copyTo(draw_templ_roi);
    cv::line(image_draw, cv::Point(raw_width, 0), cv::Point(result.box.x, result.box.y), color, 1);

    if (save_draw_) {
        save_image(image_draw);
    }
}

void Matcher::filter(ResultsVec& results, double threshold) const
{
    for (auto iter = results.begin(); iter != results.end();) {
        auto& res = *iter;

        if (res.score < threshold) {
            iter = results.erase(iter);
            continue;
        }
        ++iter;
    }
}

MAA_VISION_NS_END
