#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCV.hpp"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

enum class ResizeMode
{
    Stretch,
    Letterbox,
};

enum class ColorOrder
{
    BGR,
    RGB,
};

struct PreprocessSpec
{
    cv::Size input_size;
    ResizeMode resize_mode = ResizeMode::Stretch;
    int interpolation = cv::INTER_LINEAR;
    ColorOrder color_order = ColorOrder::RGB;
    float scale = 1.0F / 255.0F;
    std::array<float, 3> mean = { 0.0F, 0.0F, 0.0F };
    std::array<float, 3> std = { 1.0F, 1.0F, 1.0F };
    std::array<float, 3> padding_value = { 0.0F, 0.0F, 0.0F };
};

struct CoordinateTransform
{
    cv::Size source_size;
    cv::Size input_size;
    float scale_x = 1.0F;
    float scale_y = 1.0F;
    float padding_x = 0.0F;
    float padding_y = 0.0F;

    cv::Rect2f to_source(const cv::Rect2f& box) const;
};

struct PreprocessedImage
{
    std::vector<float> tensor;
    std::array<int64_t, 4> shape = { 1, 3, 0, 0 };
    CoordinateTransform transform;
};

class ImagePreprocessor
{
public:
    static bool preprocess(const cv::Mat& image, const PreprocessSpec& spec, PreprocessedImage& output, std::string& error);
};

} // namespace NeuralNetwork

MAA_VISION_NS_END
