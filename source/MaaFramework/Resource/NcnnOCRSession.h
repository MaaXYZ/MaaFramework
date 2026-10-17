#pragma once

#ifdef MAA_WITH_NCNN

#include <filesystem>
#include <memory>
#include <mutex>

#include <net.h>

#include "Common/Conf.h"
#include "Vision/OCRSession.h"

MAA_RES_NS_BEGIN

// PP-OCR (DBNet det + CTC rec) on ncnn, aligned with fastdeploy PPOCRv4 (no cls):
// preprocess, DB postprocess (incl. clipper unclip), box sorting, crop, rec batching and CTC decoding
// follow fastdeploy so that results match the onnxruntime path.
class NcnnOCRSession : public MAA_VISION_NS::OCRSession
{
public:
    inline static const std::filesystem::path kDetParamFilename = "det.ncnn.param";
    inline static const std::filesystem::path kDetBinFilename = "det.ncnn.bin";
    inline static const std::filesystem::path kRecParamFilename = "rec.ncnn.param";
    inline static const std::filesystem::path kRecBinFilename = "rec.ncnn.bin";
    // optional, only "options" (ncnn::Option overrides) is read; pre / post processing follows fastdeploy
    inline static const std::filesystem::path kDetSidecarFilename = "det.ncnn.json";
    inline static const std::filesystem::path kRecSidecarFilename = "rec.ncnn.json";
    inline static const std::filesystem::path kKeysFilename = "keys.txt";

    // rec + keys are required, det is optional (rec-only model). returns nullptr on failure.
    static std::shared_ptr<NcnnOCRSession> create(const std::filesystem::path& dir, int num_threads);

    virtual ~NcnnOCRSession() override;

public:
    virtual int det_max_side_len() const override;
    virtual std::optional<MAA_VISION_NS::OCRDetRecResult> det_and_rec(const cv::Mat& image) override;
    virtual std::optional<MAA_VISION_NS::OCRRecResult> rec(const cv::Mat& image) override;
    virtual std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> batch_rec(const std::vector<cv::Mat>& images) override;

private:
    NcnnOCRSession() = default;

    bool load(const std::filesystem::path& dir, int num_threads);
    bool load_keys(const std::filesystem::path& path);

    // fastdeploy DBDetector::Predict, boxes in original image coordinates
    std::optional<std::vector<std::array<int, 8>>> detect(const cv::Mat& image);

    // fastdeploy Recognizer::BatchPredict on one batch: all images share the padded width
    std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> rec_batch(const std::vector<const cv::Mat*>& images);
    std::optional<MAA_VISION_NS::OCRRecResult> rec_one(const cv::Mat& image, int img_w);

    std::unique_ptr<ncnn::Net> det_;
    std::unique_ptr<ncnn::Net> rec_;
    std::mutex mutex_;

    // "#"(ctc blank) + keys + " "
    std::vector<std::string> labels_;
};

MAA_RES_NS_END

#endif // MAA_WITH_NCNN
