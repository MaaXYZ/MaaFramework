#pragma once

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/TaskResultTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "PipelineTypes.h"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

class ResourceMgr;

class ResourceRecognizer
{
public:
    explicit ResourceRecognizer(ResourceMgr* resource, const cv::Mat& image);

public:
    // Unified recognition interface
    MAA_TASK_NS::RecoResult recognize(Recognition::Type type, const Recognition::Param& param);

private:
    MAA_TASK_NS::RecoResult template_match(const MAA_VISION_NS::TemplateMatcherParam& param);
    MAA_TASK_NS::RecoResult feature_match(const MAA_VISION_NS::FeatureMatcherParam& param);
    MAA_TASK_NS::RecoResult color_match(const MAA_VISION_NS::ColorMatcherParam& param);
    MAA_TASK_NS::RecoResult ocr(const MAA_VISION_NS::OCRerParam& param);
    MAA_TASK_NS::RecoResult nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param);
    MAA_TASK_NS::RecoResult nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param);

    std::vector<cv::Mat> get_images(const std::vector<std::string>& names);
    cv::Rect get_roi(const MAA_VISION_NS::Target& roi);
    void save_draws(const std::string& name, const MAA_TASK_NS::RecoResult& result) const;

private:
    bool debug_mode() const;

private:
    ResourceMgr* resource_ = nullptr;
    cv::Mat image_;
};

MAA_RES_NS_END
