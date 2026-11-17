#pragma once

#include <ostream>
#include <string>
#include <vector>

#include <boost/regex.hpp>

#include "Common/Conf.h"

#include "OCRTypes.h"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct ColorFilterConfig
{
    int method = ColorMatcherParam::kDefaultMethod;
    std::vector<ColorMatcherParam::Range> range;
};

class OCRer
    : public VisionBase
    , public RecoResultAPI<OCRerResult>
{
public:
    OCRer(
        cv::Mat image,
        std::vector<cv::Rect> rois,
        OCRerParam param,
        OCRSessionPtr session,
        std::string name = "",
        std::optional<ColorFilterConfig> color_filter = std::nullopt,
        OCRCache* cache = nullptr);

private:
    void analyze();

    std::optional<ResultsVec> predict() const;

    void add_results(ResultsVec results, const std::vector<std::wstring>& expected);
    void cherry_pick();

private:
    cv::Mat apply_color_filter(const cv::Mat& image_roi) const;
    std::optional<ResultsVec> predict_det_and_rec(const cv::Mat& image_roi) const;
    std::optional<Result> predict_only_rec(const cv::Mat& image_roi) const;

    cv::Mat draw_result(const ResultsVec& results) const;

    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;
    bool filter_by_required(const Result& res, const std::vector<std::wstring>& expected) const;
    void sort_(ResultsVec& results) const;

private:
    static const boost::wregex& gen_regex(const std::wstring& pattern);

private:
    const OCRerParam param_;
    const std::optional<ColorFilterConfig> color_filter_;

    OCRCache* cache_ = nullptr;

    OCRSessionPtr session_ = nullptr;
};

MAA_VISION_NS_END
