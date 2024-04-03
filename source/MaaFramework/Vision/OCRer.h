#pragma once

#include <ostream>
#include <vector>

#include "Conf/Conf.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "Instance/InstanceStatus.h"
#include "Utils/Codec.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class OCRer : public VisionBase
{
public:
    struct Result
    {
        std::wstring text;
        cv::Rect box {};
        double score = 0.0;

        MEO_JSONIZATION(text, box, score);
    };

    using ResultsVec = std::vector<Result>;
    using Cache = std::map<cv::Rect, ResultsVec, RectComparer>;

public:
    OCRer(
        cv::Mat image,
        OCRerParam param,
        std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
        std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
        std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer,
        Cache& cache,
        std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();

    ResultsVec predict_all_rois();
    ResultsVec predict(const cv::Rect& roi);

    void add_results(ResultsVec results, const std::vector<std::wstring>& expected);
    void cherry_pick();

private:
    ResultsVec predict_det_and_rec(const cv::Mat& image_roi) const;
    Result predict_only_rec(const cv::Mat& image_roi) const;

    cv::Mat draw_result(const cv::Rect& roi, const ResultsVec& results) const;

    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;
    bool filter_by_required(const Result& res, const std::vector<std::wstring>& expected) const;
    void sort_(ResultsVec& results) const;

private:
    const OCRerParam param_;

    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter_ = nullptr;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer_ = nullptr;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer_ = nullptr;

    Cache& cache_;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
