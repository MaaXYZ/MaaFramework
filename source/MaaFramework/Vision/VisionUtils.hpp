#pragma once

#include <numeric>
#include <random>
#include <ranges>

#include <boost/regex.hpp>

#include "Common/Conf.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

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
inline static void sort_by_score_(ResultsVec& results, bool reverse = false)
{
    if (reverse) {
        std::ranges::sort(results, std::less {}, std::mem_fn(&ResultsVec::value_type::score));
    }
    else {
        std::ranges::sort(results, std::greater {}, std::mem_fn(&ResultsVec::value_type::score));
    }
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

template <typename ResultsVec>
inline static void sort_by_expected_index_(ResultsVec& results, const std::vector<int>& expected)
{
    if (results.empty() || expected.empty()) {
        return;
    }

    std::unordered_map<size_t, size_t> index_cache;
    for (size_t i = 0; i != expected.size(); ++i) {
        index_cache.emplace(static_cast<size_t>(expected[i]), i + 1);
    }

    std::ranges::sort(results, [&index_cache](const auto& lhs, const auto& rhs) -> bool {
        auto l_it = index_cache.find(lhs.cls_index);
        auto r_it = index_cache.find(rhs.cls_index);

        bool l_matched = (l_it != index_cache.end());
        bool r_matched = (r_it != index_cache.end());

        if (!l_matched) {
            return false; // 未匹配的排后面
        }
        if (!r_matched) {
            return true; // 已匹配的排前面
        }

        return l_it->second < r_it->second; // 按预期顺序排序
    });
}

template <typename ResultsVec>
inline static void sort_by_expected_regex_(ResultsVec& results, const std::vector<std::wstring>& expected)
{
    if (results.empty() || expected.empty()) {
        return;
    }

    std::vector<boost::wregex> patterns;
    patterns.reserve(expected.size());
    for (const auto& pattern : expected) {
        patterns.emplace_back(pattern);
    }

    // 预先计算所有结果的匹配索引 (1-based，0 表示未匹配)
    std::vector<size_t> match_indices;
    match_indices.reserve(results.size());

    for (const auto& result : results) {
        size_t match_index = 0;
        for (size_t j = 0; j < patterns.size(); ++j) {
            if (boost::regex_search(result.text, patterns[j])) {
                match_index = j + 1;
                break;
            }
        }
        match_indices.push_back(match_index);
    }

    // 创建排列索引并排序
    std::vector<size_t> order(results.size());
    std::iota(order.begin(), order.end(), 0);

    std::ranges::sort(order, [&match_indices](size_t a, size_t b) -> bool {
        const size_t a_match = match_indices[a];
        const size_t b_match = match_indices[b];

        if (a_match == 0) {
            return false; // a 未匹配，排后面
        }
        if (b_match == 0) {
            return true;          // b 未匹配，a 排前面
        }
        return a_match < b_match; // 按匹配顺序排序
    });

    // 根据排序后的索引重排结果
    ResultsVec sorted_results;
    sorted_results.reserve(results.size());
    for (size_t idx : order) {
        sorted_results.push_back(std::move(results[idx]));
    }
    results = std::move(sorted_results);
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
inline static ResultsVec NMS(ResultsVec results, double threshold = 0.7, bool greater = true)
{
    std::ranges::sort(results, [&](const auto& a, const auto& b) { return greater ? (a.score > b.score) : (a.score < b.score); });

    ResultsVec nms_results;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res1 = results[i];
        if ((greater && res1.score < 0.1f) || (!greater && res1.score > 0.9f)) {
            continue;
        }
        auto res1_box = res1.box;
        nms_results.emplace_back(std::move(res1));

        for (size_t j = i + 1; j < results.size(); ++j) {
            auto& res2 = results[j];
            if ((greater && res2.score < 0.1f) || (!greater && res2.score > 0.9f)) {
                continue;
            }
            int iou_area = (res1_box & res2.box).area();
            if (iou_area >= threshold * res2.box.area()) {
                res2.score = greater ? 0 : 1;
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
    if (output.empty()) {
        return output;
    }
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

inline std::vector<cv::Rect> correct_rois(std::vector<cv::Rect> rois, const cv::Mat& image)
{
    if (rois.empty()) {
        return { correct_roi(cv::Rect {}, image) };
    }
    for (auto& roi : rois) {
        roi = correct_roi(roi, image);
    }
    return rois;
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

MAA_VISION_NS_END
