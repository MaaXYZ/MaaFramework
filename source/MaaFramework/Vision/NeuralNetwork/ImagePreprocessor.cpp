#include "ImagePreprocessor.h"

#include <algorithm>
#include <cmath>

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

cv::Rect2f CoordinateTransform::to_source(const cv::Rect2f& box) const
{
    return {
        (box.x - padding_x) / scale_x,
        (box.y - padding_y) / scale_y,
        box.width / scale_x,
        box.height / scale_y,
    };
}

bool ImagePreprocessor::preprocess(const cv::Mat& image, const PreprocessSpec& spec, PreprocessedImage& output, std::string& error)
{
    if (image.empty()) {
        error = "input image is empty";
        return false;
    }
    if (image.type() != CV_8UC3) {
        error = "input image must be CV_8UC3";
        return false;
    }
    if (spec.input_size.width <= 0 || spec.input_size.height <= 0) {
        error = "input size must be positive";
        return false;
    }
    if (std::ranges::any_of(spec.std, [](float value) { return value == 0.0F || !std::isfinite(value); })) {
        error = "normalization std must be finite and non-zero";
        return false;
    }

    cv::Mat resized;
    CoordinateTransform transform {
        .source_size = image.size(),
        .input_size = spec.input_size,
    };

    if (spec.resize_mode == ResizeMode::Stretch) {
        cv::resize(image, resized, spec.input_size, 0.0, 0.0, spec.interpolation);
        transform.scale_x = static_cast<float>(spec.input_size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(spec.input_size.height) / static_cast<float>(image.rows);
    }
    else {
        const float scale = std::min(
            static_cast<float>(spec.input_size.width) / static_cast<float>(image.cols),
            static_cast<float>(spec.input_size.height) / static_cast<float>(image.rows));
        const cv::Size resized_size {
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.cols) * scale))),
            std::max(1, static_cast<int>(std::round(static_cast<float>(image.rows) * scale))),
        };
        cv::Mat scaled;
        cv::resize(image, scaled, resized_size, 0.0, 0.0, spec.interpolation);
        transform.scale_x = static_cast<float>(resized_size.width) / static_cast<float>(image.cols);
        transform.scale_y = static_cast<float>(resized_size.height) / static_cast<float>(image.rows);
        transform.padding_x = static_cast<float>(spec.input_size.width - resized_size.width) / 2.0F;
        transform.padding_y = static_cast<float>(spec.input_size.height - resized_size.height) / 2.0F;

        const int left = static_cast<int>(std::floor(transform.padding_x));
        const int top = static_cast<int>(std::floor(transform.padding_y));
        const int right = spec.input_size.width - resized_size.width - left;
        const int bottom = spec.input_size.height - resized_size.height - top;
        const cv::Scalar padding = spec.color_order == ColorOrder::RGB
                                       ? cv::Scalar(spec.padding_value[2], spec.padding_value[1], spec.padding_value[0])
                                       : cv::Scalar(spec.padding_value[0], spec.padding_value[1], spec.padding_value[2]);
        cv::copyMakeBorder(scaled, resized, top, bottom, left, right, cv::BORDER_CONSTANT, padding);
        transform.padding_x = static_cast<float>(left);
        transform.padding_y = static_cast<float>(top);
    }

    if (spec.color_order == ColorOrder::RGB) {
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    }

    cv::Mat float_image;
    resized.convertTo(float_image, CV_32FC3, spec.scale);

    std::array<cv::Mat, 3> channels;
    cv::split(float_image, channels);
    const size_t plane_size = static_cast<size_t>(spec.input_size.area());
    output.tensor.resize(plane_size * channels.size());
    for (size_t channel = 0; channel < channels.size(); ++channel) {
        channels[channel] = (channels[channel] - spec.mean[channel]) / spec.std[channel];
        std::copy_n(channels[channel].ptr<float>(), plane_size, output.tensor.begin() + static_cast<std::ptrdiff_t>(channel * plane_size));
    }

    output.shape = { 1, 3, spec.input_size.height, spec.input_size.width };
    output.transform = transform;
    error.clear();
    return true;
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
