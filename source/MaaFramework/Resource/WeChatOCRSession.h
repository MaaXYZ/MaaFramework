#pragma once

#include <memory>

#include "Common/Conf.h"
#include "MaaUtils/JsonExt.hpp"
#include "Vision/OCRSession.h"

MAA_RES_NS_BEGIN

class WeChatOCRSession final : public MAA_VISION_NS::OCRSession
{
public:
    static std::shared_ptr<WeChatOCRSession> create(const json::value& config);
    ~WeChatOCRSession() override;

    bool supports_rec_only() const override { return false; }

    bool supports_batch_cache() const override { return false; }

    int det_max_side_len() const override { return 0; }

    std::optional<MAA_VISION_NS::OCRDetRecResult> det_and_rec(const cv::Mat& image) override;
    std::optional<MAA_VISION_NS::OCRRecResult> rec(const cv::Mat& image) override;
    std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> batch_rec(const std::vector<cv::Mat>& images) override;

private:
    struct Impl;
    explicit WeChatOCRSession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

MAA_RES_NS_END
