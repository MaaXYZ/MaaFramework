#pragma once

#include <ostream>
#include <vector>

#include "Conf/Conf.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/Codec.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct OCRerResult
{
    std::wstring text;
    cv::Rect box {};
    double score = 0.0;

    MEO_JSONIZATION(text, box, score);
};

class OCRer
    : public VisionBase
    , public RecoResultAPI<OCRerResult>
{
public:
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

private:
    void analyze();

    ResultsVec predict_all_rois() const;
    ResultsVec predict(const cv::Rect& roi) const;

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
};

MAA_VISION_NS_END
