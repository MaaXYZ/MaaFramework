#include "OCRer.h"

#include <regex>

#include "Resource/ResourceMgr.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

MAA_VISION_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const OCRer::Result& res)
{
    os << VAR_RAW(res.text) << VAR_RAW(res.box) << VAR_RAW(res.score);
    return os;
}

OCRer::ResultOpt OCRer::analyze() const
{
    auto results = predict();

    if (results.empty()) {
        return std::nullopt;
    }

    for (auto& res : results) {
        postproc_trim_(res);
        postproc_replace_(res);
    }

    // TODO
    //LogTrace << VAR(results);

    return results;
}

OCRer::ResultsVec OCRer::predict() const
{
    fastdeploy::vision::OCRResult ocr_result;
    auto& inferencer = resource()->ocr_cfg().ocrer();
    if (!inferencer) {
        LogError << "resource()->ocr_cfg().ocrer() is null";
        return {};
    }
    auto start_time = std::chrono::steady_clock::now();

    auto image_roi = image_(roi_);
    bool ret = inferencer->Predict(image_roi, &ocr_result);
    if (!ret) {
        LogWarn << "inferencer return false" << VAR(inferencer) << VAR(image_);
        return {};
    }
    if (ocr_result.boxes.size() != ocr_result.text.size()) {
        LogError << "ocr_result.boxes.size() != ocr_result.text.size()" << VAR(ocr_result.boxes.size())
                 << VAR(ocr_result.text.size());
        return {};
    }

    ResultsVec results;
    for (size_t i = 0; i != ocr_result.text.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = ocr_result.boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = ranges::minmax(x_collect);
        auto [top, bottom] = ranges::minmax(y_collect);

        cv::Rect my_box(left + roi_.x, top + roi_.y, right - left, bottom - top);
#ifdef MAA_DEBUG
        cv::rectangle(image_draw_, my_box, cv::Scalar(0, 0, 255), 2);
#endif

        results.emplace_back(
            Result { .text = std::move(ocr_result.text.at(i)), .box = my_box, .score = ocr_result.rec_scores.at(i) });
    }
    auto costs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    // TODO
    //LogTrace << VAR(results) << VAR(image_roi) << VAR(costs);

    return results;
}

void OCRer::postproc_trim_(Result& res) const
{
    string_trim_(res.text);
}

void OCRer::postproc_replace_(Result& res) const
{
    for (const auto& [regex, new_str] : param_.replace) {
        res.text = std::regex_replace(res.text, std::regex(regex), new_str);
    }
}

MAA_VISION_NS_END
