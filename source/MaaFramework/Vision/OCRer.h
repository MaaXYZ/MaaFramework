#pragma once

#include <mutex>
#include <ostream>
#include <vector>

#include <boost/regex.hpp>

#include "Common/Conf.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

namespace fastdeploy
{
namespace vision::ocr
{
class DBDetector;
class Recognizer;
}

namespace pipeline
{
class PPOCRv3;
}
}

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
    OCRer(
        cv::Mat image,
        std::vector<cv::Rect> rois,
        OCRerParam param,
        std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
        std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
        std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer,
        std::string name = "");

private:
    void analyze();

    ResultsVec predict() const;

    void add_results(ResultsVec results, const std::vector<std::wstring>& expected);
    void cherry_pick();

private:
    ResultsVec predict_det_and_rec(const cv::Mat& image_roi) const;
    Result predict_only_rec(const cv::Mat& image_roi) const;

    cv::Mat draw_result(const ResultsVec& results) const;

    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;
    bool filter_by_required(const Result& res, const std::vector<std::wstring>& expected) const;
    void sort_(ResultsVec& results) const;

private:
    static const boost::wregex& gen_regex(const std::wstring& pattern);

private:
    const OCRerParam param_;

    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter_ = nullptr;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer_ = nullptr;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer_ = nullptr;

    inline static std::mutex s_predict_mutex_;
};

MAA_VISION_NS_END
