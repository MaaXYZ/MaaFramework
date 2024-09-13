#pragma once

#include <random>
#include <ranges>

#include "Conf/Conf.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_VISION_NS_BEGIN

// | 1 3 5 7 |
// | 2 4 6 8 |
template <typename ResultsVec>
inline static void sort_by_horizontal_(ResultsVec& results)
{
    std::ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        return lhs.box.x == rhs.box.x ? lhs.box.y < rhs.box.y : lhs.box.x < rhs.box.x;
    });
}

// | 1 2 3 4 |
// | 5 6 7 8 |
template <typename ResultsVec>
inline static void sort_by_vertical_(ResultsVec& results)
{
    std::ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        return lhs.box.y == rhs.box.y ? lhs.box.x < rhs.box.x : lhs.box.y < rhs.box.y;
    });
}

template <typename ResultsVec>
inline static void sort_by_score_(ResultsVec& results)
{
    std::ranges::sort(results, std::greater {}, std::mem_fn(&ResultsVec::value_type::score));
}

template <typename ResultsVec>
inline static void sort_by_count_(ResultsVec& results)
{
    std::ranges::sort(results, std::greater {}, std::mem_fn(&ResultsVec::value_type::count));
}

template <typename ResultsVec>
inline static void sort_by_area_(ResultsVec& results)
{
    std::ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool { return lhs.box.area() > rhs.box.area(); });
}

template <typename ResultsVec>
inline static void sort_by_random_(ResultsVec& results)
{
    static std::default_random_engine rand_engine(std::random_device {}());
    std::ranges::shuffle(results, rand_engine);
}

template <typename ResultsVec>
inline static void sort_by_required_(ResultsVec& results, const std::vector<std::string>& required)
{
    std::unordered_map<std::string, size_t> req_cache;
    for (size_t i = 0; i != required.size(); ++i) {
        req_cache.emplace(required.at(i), i + 1);
    }

    // 不在 required 中的将被排在最后
    std::ranges::sort(results, [&req_cache](const auto& lhs, const auto& rhs) -> bool {
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

inline static std::optional<size_t> pythonic_index(size_t total, int index)
{
    if (index >= 0 && static_cast<uint32_t>(index) < total) {
        return index;
    }
    if (index < 0 && static_cast<uint32_t>(-index) <= total) {
        return total + index;
    }
    return std::nullopt;
}

// Non-Maximum Suppression
template <typename ResultsVec>
inline static ResultsVec NMS(ResultsVec results, double threshold = 0.7)
{
    std::ranges::sort(results, [](const auto& a, const auto& b) { return a.score > b.score; });

    ResultsVec nms_results;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res1 = results[i];
        if (res1.score < 0.1f) {
            continue;
        }
        auto res1_box = res1.box;
        nms_results.emplace_back(std::move(res1));

        for (size_t j = i + 1; j < results.size(); ++j) {
            auto& res2 = results[j];
            if (res2.score < 0.1f) {
                continue;
            }
            int iou_area = (res1_box & res2.box).area();
            if (iou_area >= threshold * res2.box.area()) {
                res2.score = 0;
            }
        }
    }
    return nms_results;
}

template <typename ResultsVec>
inline static ResultsVec NMS_for_count(ResultsVec results, double threshold = 0.7)
{
    std::ranges::sort(results, [](const auto& a, const auto& b) { return a.count > b.count; });

    ResultsVec nms_results;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res1 = results[i];
        if (res1.count == 0) {
            continue;
        }
        auto res1_box = res1.box;
        nms_results.emplace_back(std::move(res1));

        for (size_t j = i + 1; j < results.size(); ++j) {
            auto& res2 = results[j];
            if (res2.count == 0) {
                continue;
            }
            int iou_area = (res1_box & res2.box).area();
            if (iou_area >= threshold * res2.box.area()) {
                res2.count = 0;
            }
        }
    }
    return nms_results;
}

template <typename T>
inline static T softmax(const T& input)
{
    T output = input;
    float rowmax = *std::ranges::max_element(output);
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

template <typename ResultsVec>
inline static void merge_vector_(ResultsVec& left, ResultsVec right)
{
    left.insert(left.end(), std::make_move_iterator(right.begin()), std::make_move_iterator(right.end()));
}

inline static cv::Mat hwc_to_chw(const cv::Mat& src)
{
    std::vector<cv::Mat> rgb_images;
    cv::split(src, rgb_images);

    // Stretch one-channel image to vector
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
        LogError << "image is empty" << VAR(image.size());
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

inline cv::Mat create_mask(const cv::Mat& image, bool green_mask)
{
    cv::Mat mask = cv::Mat::ones(image.size(), CV_8UC1);
    if (green_mask) {
        cv::inRange(image, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
        mask = ~mask;
    }
    return mask;
}

inline cv::Mat create_mask(const cv::Mat& image, const cv::Rect& roi)
{
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    mask(roi) = 255;
    return mask;
}

inline std::ostream& operator<<(std::ostream& os, const cv::Rect& rect)
{
    os << "Rect(" << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << ")";
    return os;
}

MAA_VISION_NS_END
