#include "NeuralNetworkDetector.h"

#include <ranges>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Utils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

NeuralNetworkDetector::NeuralNetworkDetector(
    cv::Mat image,
    cv::Rect roi,
    NeuralNetworkDetectorParam param,
    std::shared_ptr<Ort::Session> session,
    const Ort::MemoryInfo& memory_info,
    std::string name)
    : VisionBase(std::move(image), std::move(roi), std::move(name))
    , param_(std::move(param))
    , session_(std::move(session))
    , memory_info_(memory_info)
{
    analyze();
}

void NeuralNetworkDetector::analyze()
{
    LogFunc << name_ << VAR(uid_);

    if (!session_) {
        LogError << "OrtSession not loaded";
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    auto results = detect();
    add_results(std::move(results), param_.expected, param_.thresholds);

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model)
             << VAR(param_.labels) << VAR(param_.expected) << VAR(param_.thresholds);
}

NeuralNetworkDetector::ResultsVec NeuralNetworkDetector::detect() const
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
            res.label = res.cls_index < param_.labels.size() ? param_.labels[res.cls_index] : std::format("Unkonwn_{}", res.cls_index);
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
    if (expected.size() != thresholds.size()) {
        LogError << name_ << VAR(uid_) << "expected.size() != thresholds.size()" << VAR(expected) << VAR(thresholds);
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
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

MAA_VISION_NS_END
