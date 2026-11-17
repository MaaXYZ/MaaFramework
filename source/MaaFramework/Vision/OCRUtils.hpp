#pragma once

#include <algorithm>

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "OCRTypes.h"

MAA_VISION_NS_BEGIN

inline std::optional<std::vector<OCRerResult>> decode_ocr_result(const OCRDetRecResult& prediction, const cv::Size& image_size)
{
    const auto& [boxes, texts, scores] = prediction;

    std::vector<OCRerResult> results;

    if (boxes.size() != texts.size() || texts.size() != scores.size()) {
        LogWarn << "Wrong ocr_result size" << VAR(boxes) << VAR(texts) << VAR(scores);

        if (boxes.empty() && texts.size() == 1 && scores.size() == 1) {
            if (const auto& raw_text = texts.front(); !raw_text.empty()) {
                // 这种情况是 det 模型没出结果，整个 ROI 直接被送给了 rec 模型。凑合用吧（
                results.emplace_back(
                    OCRerResult { .text = to_u16(raw_text),
                                  .box = { 0, 0, image_size.width, image_size.height },
                                  .score = scores.front() });
            }
            return results;
        }

        return std::nullopt;
    }

    for (size_t i = 0; i != texts.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = std::ranges::minmax(x_collect);
        auto [top, bottom] = std::ranges::minmax(y_collect);

        cv::Rect my_box(left, top, right - left, bottom - top);
        results.emplace_back(OCRerResult { .text = to_u16(texts.at(i)), .box = my_box, .score = scores.at(i) });
    }

    return results;
}

MAA_VISION_NS_END
