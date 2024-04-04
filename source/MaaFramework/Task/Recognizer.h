#pragma once

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Vision/OCRer.h"

MAA_TASK_NS_BEGIN

class Recognizer
{
public:
    using TaskData = MAA_RES_NS::TaskData;

    using Hit = cv::Rect;

    struct Result
    {
        MaaRecoId uid = 0;
        std::optional<Hit> hit = std::nullopt;
        json::value detail;
        std::vector<cv::Mat> draws;
    };

public:
    explicit Recognizer(InstanceInternalAPI* inst);

public:
    Result recognize(const cv::Mat& image, const TaskData& task_data);

    static bool query_detail(
        MaaRecoId reco_id,
        bool& hit,
        cv::Rect& box,
        std::string& detail,
        std::vector<cv::Mat>& draws);

private:
    Result direct_hit(const std::string& name);
    Result template_match(
        const cv::Mat& image,
        const MAA_VISION_NS::TemplateMatcherParam& param,
        const std::string& name);
    Result feature_match(
        const cv::Mat& image,
        const MAA_VISION_NS::FeatureMatcherParam& param,
        const std::string& name);
    Result color_match(
        const cv::Mat& image,
        const MAA_VISION_NS::ColorMatcherParam& param,
        const std::string& name);
    Result
        ocr(const cv::Mat& image, const MAA_VISION_NS::OCRerParam& param, const std::string& name);
    Result nn_classify(
        const cv::Mat& image,
        const MAA_VISION_NS::NeuralNetworkClassifierParam& param,
        const std::string& name);
    Result nn_detect(
        const cv::Mat& image,
        const MAA_VISION_NS::NeuralNetworkDetectorParam& param,
        const std::string& name);
    Result custom_recognize(
        const cv::Mat& image,
        const MAA_VISION_NS::CustomRecognizerParam& param,
        const std::string& name);

    void save_draws(const std::string& task_name, const Result& result) const;
    void show_hit_draw(
        const cv::Mat& image,
        const Hit& res,
        const std::string& task_name,
        MaaRecoId uid) const;

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

private:
    InstanceInternalAPI* inst_ = nullptr;
    MAA_VISION_NS::OCRer::Cache ocr_cache_;
};

MAA_TASK_NS_END
