#pragma once

#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"

#include <stack>

MAA_TASK_NS_BEGIN

class Recognizer
{
public:
    using TaskData = MAA_RES_NS::TaskData;

    struct Result
    {
        cv::Rect box {};
        json::value detail;
    };

public:
    Recognizer(InstanceInternalAPI* inst);

public:
    std::optional<Result> recognize(const cv::Mat& image, const TaskData& task_data);

private:
    std::optional<Result> direct_hit(const std::string& name);
    std::optional<Result> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplateMatcherParam& param,
                                         const std::string& name);
    std::optional<Result> feature_match(const cv::Mat& image, const MAA_VISION_NS::FeatureMatcherParam& param,
                                        const std::string& name);
    std::optional<Result> color_match(const cv::Mat& image, const MAA_VISION_NS::ColorMatcherParam& param,
                                      const std::string& name);
    std::optional<Result> ocr(const cv::Mat& image, const MAA_VISION_NS::OCRerParam& param, const std::string& name);
    std::optional<Result> classify(const cv::Mat& image, const MAA_VISION_NS::NeuralNetworkClassifierParam& param,
                                   const std::string& name);
    std::optional<Result> detect(const cv::Mat& image, const MAA_VISION_NS::NeuralNetworkDetectorParam& param,
                                 const std::string& name);
    std::optional<Result> custom_recognize(const cv::Mat& image, const MAA_VISION_NS::CustomRecognizerParam& param,
                                           const std::string& name);

private:
    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

private:
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
