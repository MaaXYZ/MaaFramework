#pragma once

#include "Conf/Conf.h"

#include <ostream>
#include <vector>

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/Codec.h"
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

        operator json::value() const
        {
            json::value root;
            root["text"] = from_u16(text);
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["score"] = score;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    void set_session(std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
                     std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
                     std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer)
    {
        deter_ = std::move(deter);
        recer_ = std::move(recer);
        ocrer_ = std::move(ocrer);
    }
    void set_param(OCRerParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois() const;
    ResultsVec predict(const cv::Rect& roi) const;
    ResultsVec predict_det_and_rec(const cv::Rect& roi) const;
    Result predict_only_rec(const cv::Rect& roi) const;
    void draw_result(const cv::Rect& roi, const ResultsVec& results) const;

    void postproc_and_filter(ResultsVec& results, const std::vector<std::wstring>& expected) const;
    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;
    bool filter_by_required(const Result& res, const std::vector<std::wstring>& expected) const;

    OCRerParam param_;
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter_ = nullptr;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer_ = nullptr;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer_ = nullptr;
};

MAA_VISION_NS_END
