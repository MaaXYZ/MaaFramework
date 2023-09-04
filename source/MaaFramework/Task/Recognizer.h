#pragma once

#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Task/TaskInstAPI.h"

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
    Recognizer(TaskInstAPI& task_inst);

public:
    std::optional<Result> recognize(const cv::Mat& image, const TaskData& task_data);

private:
    std::optional<Result> direct_hit();
    std::optional<Result> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplMatchingParam& param,
                                         const cv::Rect& cache, const std::string& name);
    std::optional<Result> ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParam& param, const cv::Rect& cache,
                              const std::string& name);
    std::optional<Result> custom_recognize(const cv::Mat& image, const MAA_VISION_NS::CustomParam& param,
                                           const cv::Rect& cache, const std::string& name);

private:
    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }

private:
    TaskInstAPI& task_inst_;
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
