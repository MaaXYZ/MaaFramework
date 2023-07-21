#pragma once

#include "Common/MaaConf.h"
#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"

MAA_VISION_NS_BEGIN

// | 1 2 3 4 |
// | 5 6 7 8 |
template <typename ResultsVec>
inline static void sort_by_horizontal_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // y 差距较小则理解为是同一排的，按x排序
        return std::abs(lhs.rect.y - rhs.rect.y) < 5 ? lhs.rect.x < rhs.rect.x : lhs.rect.y < rhs.rect.y;
    });
}

// | 1 3 5 7 |
// | 2 4 6 8 |
template <typename ResultsVec>
inline static void sort_by_vertical_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // x 差距较小则理解为是同一排的，按y排序
        return std::abs(lhs.rect.x - rhs.rect.x) < 5 ? lhs.rect.y < rhs.rect.y : lhs.rect.x < rhs.rect.x;
    });
}

template <typename ResultsVec>
inline static void sort_by_score_(ResultsVec& results)
{
    ranges::sort(results, std::greater {}, std::mem_fn(&ResultsVec::value_type::score));
}

template <typename ResultsVec>
inline static void sort_by_required_(ResultsVec& results, const std::vector<std::string>& required)
{
    std::unordered_map<std::string, size_t> req_cache;
    for (size_t i = 0; i != required.size(); ++i) {
        req_cache.emplace(required.at(i), i + 1);
    }

    // 不在 required 中的将被排在最后
    ranges::sort(results, [&req_cache](const auto& lhs, const auto& rhs) -> bool {
        size_t lvalue = req_cache[lhs.text];
        size_t rvalue = req_cache[rhs.text];
        if (lvalue == 0) {
            return false;
        }
        else if (rvalue == 0) {
            return true;
        }
        return lvalue < rvalue;
    });
}

// Non-Maximum Suppression
template <typename ResultsVec>
inline static ResultsVec NMS(ResultsVec results, double threshold = 0.7)
{
    ranges::sort(results, [](const auto& a, const auto& b) { return a.score > b.score; });

    ResultsVec nms_results;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& box = results[i];
        if (box.score < 0.1f) {
            continue;
        }
        nms_results.emplace_back(box);
        for (size_t j = i + 1; j < results.size(); ++j) {
            auto& box2 = results[j];
            if (box2.score < 0.1f) {
                continue;
            }
            int iou_area = (make_rect<cv::Rect>(box.rect) & make_rect<cv::Rect>(box2.rect)).area();
            if (iou_area > threshold * box2.rect.area()) {
                box2.score = 0;
            }
        }
    }
    return nms_results;
}

template <typename T>
inline static T softmax(const T& input)
{
    T output = input;
    float rowmax = *ranges::max_element(output);
    std::vector<float> y(output.size());
    float sum = 0.0f;
    for (size_t i = 0; i != output.size(); ++i) {
        sum += y[i] = std::exp(output[i] - rowmax);
    }
    for (size_t i = 0; i != output.size(); ++i) {
        output[i] = y[i] / sum;
    }
    return output;
}

inline static cv::Mat hwc_to_chw(const cv::Mat& src)
{
    std::vector<cv::Mat> rgb_images;
    cv::split(src, rgb_images);

    // Stretch one-channel images to vector
    cv::Mat flat_r = rgb_images[0].reshape(1, 1);
    cv::Mat flat_g = rgb_images[1].reshape(1, 1);
    cv::Mat flat_b = rgb_images[2].reshape(1, 1);

    // Now we can rearrange channels if need
    cv::Mat matArray[] = { flat_r, flat_g, flat_b };

    cv::Mat flat_image;
    // Concatenate three vectors to one
    cv::hconcat(matArray, 3, flat_image);
    return flat_image;
}

inline static std::vector<float> image_to_tensor(const cv::Mat& image)
{
    cv::Mat src = image.clone();
    cv::cvtColor(src, src, cv::COLOR_BGR2RGB);

    cv::Mat chw = hwc_to_chw(src);
    cv::Mat chw_32f;
    chw.convertTo(chw_32f, CV_32F, 1.0 / 255.0);

    size_t tensor_size = 1ULL * src.cols * src.rows * src.channels();
    std::vector<float> tensor(tensor_size);
    std::memcpy(tensor.data(), chw_32f.data, tensor_size * sizeof(float));
    return tensor;
}

inline cv::Rect correct_roi(const cv::Rect& roi, const cv::Mat& image)
{
    if (image.empty()) {
        LogError << "image is empty" << VAR(image);
        return roi;
    }
    if (roi.empty()) {
        return { 0, 0, image.cols, image.rows };
    }

    cv::Rect res = roi;
    if (image.cols < res.x) {
        LogError << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.x = image.cols - res.width;
    }
    if (image.rows < res.y) {
        LogError << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.y = image.rows - res.height;
    }

    if (res.x < 0) {
        LogWarn << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.x = 0;
    }
    if (res.y < 0) {
        LogWarn << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.y = 0;
    }
    if (image.cols < res.x + res.width) {
        LogWarn << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.width = image.cols - res.x;
    }
    if (image.rows < res.y + res.height) {
        LogWarn << "roi is out of range" << VAR(image.size()) << VAR(res);
        res.height = image.rows - res.y;
    }
    return res;
}

inline cv::Mat match_template(const cv::Mat& image, const cv::Mat& templ, int method, bool green_mask)
{
    if (templ.cols > image.cols || templ.rows > image.rows) {
        LogError << "templ size is too large" << VAR(image) << VAR(templ);
        return {};
    }

    auto mask = cv::noArray();
    if (green_mask) {
        cv::inRange(templ, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
    }

    cv::Mat matched;
    cv::matchTemplate(image, templ, matched, method, mask);
    return matched;
}

MAA_VISION_NS_END

MAA_NS_BEGIN

inline std::ostream& operator<<(std::ostream& os, const cv::Rect& rect)
{
    os << "Rect(" << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << ")";
    return os;
}

MAA_NS_END
