#include "NeuralNetworkDetector.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <ranges>
#include <sstream>

#include <boost/regex.hpp>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

NeuralNetworkDetector::NeuralNetworkDetector(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    NeuralNetworkDetectorParam param,
    std::shared_ptr<Ort::Session> session,
    const Ort::MemoryInfo& memory_info,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , session_(std::move(session))
    , memory_info_(memory_info)
{
    analyze();
}

void NeuralNetworkDetector::analyze()
{
    LogFunc << name_;

    if (!session_) {
        LogError << "OrtSession not loaded";
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    auto labels = param_.labels.empty() ? parse_labels_from_metadata() : param_.labels;
    while (next_roi()) {
        auto results = detect(labels);
        add_results(std::move(results), param_.expected, param_.thresholds);
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model) << VAR(labels)
             << VAR(param_.expected) << VAR(param_.thresholds);
}

NeuralNetworkDetector::ResultsVec NeuralNetworkDetector::detect(const std::vector<std::string>& labels) const
{
    if (!session_) {
        LogError << "OrtSession not loaded";
        return {};
    }

    // batch_size, channel, height, width
    // for yolov8, input_shape is { 1, 3, 640, 640 }
    const auto input_shape = session_->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    if (input_shape.size() != 4) {
        LogError << "Input shape is not 4" << VAR(input_shape);
        return {};
    }

    cv::Mat image = image_with_roi();
    cv::Size raw_roi_size(image.cols, image.rows);
    cv::Size input_image_size(static_cast<int>(input_shape[3]), static_cast<int>(input_shape[2]));
    cv::resize(image, image, input_image_size, 0, 0, cv::INTER_AREA);
    std::vector<float> input = image_to_tensor(image);

    Ort::Value input_tensor =
        Ort::Value::CreateTensor<float>(memory_info_, input.data(), input.size(), input_shape.data(), input_shape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_0 = session_->GetInputNameAllocated(0, allocator).get();
    const std::string out_0 = session_->GetOutputNameAllocated(0, allocator).get();
    const std::vector input_names { in_0.c_str() };
    const std::vector output_names { out_0.c_str() };

    Ort::RunOptions run_options;
    auto output_tensor =
        session_->Run(run_options, input_names.data(), &input_tensor, input_names.size(), output_names.data(), output_names.size());

    const float* raw_output = output_tensor[0].GetTensorData<float>();
    // output_shape is { 1, 5, 8400 }
    std::vector<int64_t> output_shape = output_tensor[0].GetTensorTypeAndShapeInfo().GetShape();

    // yolov8 的 onnx 输出和前面的 v5, v7 等似乎不太一样，目前网上 yolov8 的 demo 较少，文档也没找到
    // 这里的输出解析是我跟着数据推测的：
    // center_x0, center_x1, ..... center_x8399
    // center_y0, center_y1, ..... center_y8399
    // w0, w1, ..... w8399
    // h0, h1, ..... h8399
    // cls1: conf0, conf1, ..... conf8399
    // cls2: conf0, conf1, ..... conf8399
    // cls3: conf0, conf1, ..... conf8399
    // ......
    std::vector<std::vector<float>> output(output_shape[1]);
    for (int64_t i = 0; i < output_shape[1]; i++) {
        output[i] = std::vector<float>(raw_output + i * output_shape[2], raw_output + (i + 1) * output_shape[2]);
    }

    ResultsVec raw_results;
    const size_t output_size = output.back().size();
    double width_ratio = 1.0 * raw_roi_size.width / input_image_size.width;
    double height_ratio = 1.0 * raw_roi_size.height / input_image_size.height;

    for (size_t i = 0; i < output_size; ++i) {
        constexpr size_t kConfidenceIndex = 4;
        for (size_t j = kConfidenceIndex; j < output.size(); ++j) {
            float score = output[j][i];
            constexpr float kThreshold = 0.3f;
            if (score < kThreshold) {
                continue;
            }

            int center_x = static_cast<int>(output[0][i]);
            int center_y = static_cast<int>(output[1][i]);
            int w = static_cast<int>(output[2][i]);
            int h = static_cast<int>(output[3][i]);

            int x = center_x - w / 2;
            int y = center_y - h / 2;
            cv::Rect box {
                static_cast<int>(x * width_ratio) + roi_.x,
                static_cast<int>(y * height_ratio) + roi_.y,
                static_cast<int>(w * width_ratio),
                static_cast<int>(h * height_ratio),
            };

            Result res;
            res.cls_index = j - kConfidenceIndex;
            res.label = res.cls_index < labels.size() ? labels[res.cls_index] : std::format("Unknown_{}", res.cls_index);
            res.box = box;
            res.score = score;

            raw_results.emplace_back(std::move(res));
        }
    }

    auto nms_results = NMS(std::move(raw_results));

    if (debug_draw_) {
        auto draw = draw_result(nms_results);
        handle_draw(draw);
    }

    return nms_results;
}

void NeuralNetworkDetector::add_results(ResultsVec results, const std::vector<int>& expected, const std::vector<double>& thresholds)
{
    if (expected.empty()) {
        // expected 为空时，所有结果均可用，但仍需满足默认阈值
        double default_threshold = thresholds.empty() ? NeuralNetworkDetectorParam::kDefaultThreshold : thresholds.front();
        std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) {
            return res.score >= default_threshold;
        });
        merge_vector_(all_results_, std::move(results));
        return;
    }

    if (expected.size() != thresholds.size()) {
        LogError << name_ << "expected.size() != thresholds.size()" << VAR(expected) << VAR(thresholds);
        return;
    }

    for (size_t i = 0; i != expected.size(); ++i) {
        int exp = expected.at(i);
        auto it = std::ranges::find(results, exp, std::mem_fn(&Result::cls_index));
        if (it == results.end()) {
            continue;
        }
        const Result& res = *it;
        double thres = thresholds.at(i);
        if (res.score < thres) {
            continue;
        }
        filtered_results_.emplace_back(res);
    }

    merge_vector_(all_results_, std::move(results));
}

void NeuralNetworkDetector::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

cv::Mat NeuralNetworkDetector::draw_result(const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();

    for (const Result& res : results) {
        const cv::Rect& my_box = res.box;

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, my_box, color, 1);
        std::string flag = std::format(
            "{} {} {:.3f}: [{}, {}, {}, {}]",
            res.cls_index,
            res.label,
            res.score,
            my_box.x,
            my_box.y,
            my_box.width,
            my_box.height);
        cv::putText(image_draw, flag, cv::Point(my_box.x, my_box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    return image_draw;
}

void NeuralNetworkDetector::sort_(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    case ResultOrderBy::Score:
        sort_by_score_(results);
        break;
    case ResultOrderBy::Area:
        sort_by_area_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    case ResultOrderBy::Expected:
        sort_by_expected_index_(results, param_.expected);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

std::vector<std::string> NeuralNetworkDetector::parse_labels_from_metadata() const
{
    if (!session_) {
        return {};
    }

    Ort::AllocatorWithDefaultOptions allocator;
    Ort::ModelMetadata metadata = session_->GetModelMetadata();

    std::string names_str;

    constexpr std::array<std::string_view, 4> possible_keys = { "names", "name", "labels", "class_names" };
    for (const std::string_view& key : possible_keys) {
        auto ptr = metadata.LookupCustomMetadataMapAllocated(key.data(), allocator);
        if (!ptr) {
            continue;
        }
        names_str = ptr.get();
        break;
    }

    if (names_str.empty()) {
        LogDebug << name_ << "No metadata found with keys: names, name, labels, class_names";
        return {};
    }

    LogDebug << name_ << "Found metadata" << VAR(names_str);

    // 解析字符串格式：{0: 'white_dog', 1: 'white_cat', 2: 'black_dog', 3: 'black_cat'}
    // 支持单引号和双引号
    std::vector<std::string> labels;

    // 解析 Python 字典格式：{0: 'label1', 1: 'label2', ...}
    // 正则表达式匹配：数字: 引号内的字符串
    // 支持单引号和双引号，以及可能的空格
    boost::regex dict_pattern(R"((\d+)\s*:\s*['"]([^'"]+)['"])");
    boost::sregex_iterator iter(names_str.begin(), names_str.end(), dict_pattern);
    boost::sregex_iterator end;

    std::map<int, std::string> label_map;
    for (; iter != end; ++iter) {
        const boost::smatch& match = *iter;
        if (match.size() < 3) {
            continue; // 跳过不完整的匹配
        }

        const std::string& index_str = match[1].str();
        int index = std::stoi(index_str);

        const std::string& label = match[2].str();
        if (label.empty()) {
            continue; // 跳过空标签
        }

        label_map[index] = label;
    }

    if (label_map.empty()) {
        LogWarn << name_ << "Failed to parse metadata as Python dict format" << VAR(names_str);
        return {};
    }

    // 找到最大索引，创建对应大小的向量
    int max_index = label_map.rbegin()->first;
    if (max_index < 0 || max_index > 10000) {
        LogWarn << name_ << "Invalid max_index" << VAR(max_index);
        return {};
    }

    labels.resize(static_cast<size_t>(max_index + 1));
    for (const auto& [index, label] : label_map) {
        if (index >= 0 && static_cast<size_t>(index) < labels.size()) {
            labels[static_cast<size_t>(index)] = label;
        }
    }

    LogDebug << name_ << "Parsed labels from metadata" << VAR(labels.size());
    return labels;
}

MAA_VISION_NS_END
