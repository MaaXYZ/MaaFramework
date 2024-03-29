#pragma once

#include <atomic>
#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class Recognizer
{
public:
    using TaskData = MAA_RES_NS::TaskData;

    struct Hit
    {
        cv::Rect box {};
        json::value detail;
    };

    struct Result
    {
    public:
        int64_t uid = ++s_global_uid;
        std::optional<Hit> hit = std::nullopt;
        std::vector<cv::Mat> draws;

    private:
        inline static std::atomic_int64_t s_global_uid = 0;
    };

public:
    explicit Recognizer(InstanceInternalAPI* inst);

public:
    Result recognize(const cv::Mat& image, const TaskData& task_data);

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

    void save_draws(const Result& result, const std::string& task_name) const;
    void show_hit_draw(const cv::Mat& image, const Hit& res, const std::string& task_name) const;

private:
    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }

    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

private:
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
