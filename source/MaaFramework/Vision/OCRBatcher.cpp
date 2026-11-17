#include "OCRBatcher.h"

#include <cstdint>
#include <map>
#include <set>
#include <tuple>

#include "MaaUtils/Logger.h"
#include "OCRUtils.hpp"

MAA_VISION_NS_BEGIN

OCRBatcher::OCRBatcher(cv::Mat image, const std::vector<cv::Rect>& rois, OCRSessionPtr session, OCRCache& cache)
    : image_(std::move(image))
    , session_(std::move(session))
    , cache_(cache)
{
    analyze(rois);
}

void OCRBatcher::analyze(const std::vector<cv::Rect>& rois)
{
    constexpr int64_t kMaxBatchPixels = 2 * 960 * 960;

    if (image_.empty() || !session_ || !session_->supports_cache() || rois.size() < kBatchSize) {
        return;
    }

    // Equal detector shapes avoid padding one crop to another crop's dimensions.
    std::map<std::pair<int, int>, std::vector<cv::Rect>> groups;
    std::set<std::tuple<int, int, int, int>> seen;
    const cv::Rect bounds(0, 0, image_.cols, image_.rows);
    for (const auto& roi : rois) {
        if (roi.empty() || (roi & bounds) != roi || cache_.find(session_, roi)
            || !seen.emplace(roi.x, roi.y, roi.width, roi.height).second) {
            continue;
        }
        auto lock = session_->lock();
        const auto shape = session_->batch_det_shape(roi.size());
        if (!shape || shape->width <= 0 || shape->height <= 0) {
            continue;
        }
        groups[{ shape->width, shape->height }].emplace_back(roi);
    }

    for (const auto& [shape, group] : groups) {
        const int64_t pixels = static_cast<int64_t>(shape.first) * shape.second;
        if (pixels > kMaxBatchPixels / static_cast<int64_t>(kBatchSize)) {
            continue;
        }
        // Only complete batches run eagerly; unmatched crops and tails stay lazy.
        for (size_t begin = 0; begin + kBatchSize <= group.size(); begin += kBatchSize) {
            predict(std::span(group).subspan(begin, kBatchSize));
        }
    }
}

void OCRBatcher::predict(std::span<const cv::Rect> rois)
{
    std::vector<cv::Mat> images;
    images.reserve(rois.size());
    for (const auto& roi : rois) {
        images.emplace_back(image_(roi));
    }

    std::optional<std::vector<OCRDetRecResult>> predictions;
    {
        auto lock = session_->lock();
        predictions = session_->batch_det_and_rec(images);
    }
    if (!predictions || predictions->size() != rois.size()) {
        LogWarn << "OCR tensor batch failed; candidates will retry independently";
        return;
    }
    for (size_t i = 0; i < rois.size(); ++i) {
        auto results = decode_ocr_result(predictions->at(i), rois[i].size());
        if (!results) {
            continue;
        }
        for (auto& result : *results) {
            result.box.x += rois[i].x;
            result.box.y += rois[i].y;
        }
        cache_.insert(session_, rois[i], std::move(*results));
    }
    LogDebug << "OCR tensor batch completed" << VAR(rois.size());
}

MAA_VISION_NS_END
