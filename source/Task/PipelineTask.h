#pragma once

#include "Resource/PipelineTypes.h"
#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;
    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual bool set_param(const json::value& param) override;

    virtual std::string_view type() const override { return "PipelineTask"; }

private:
    enum class RunningResult
    {
        Success,
        Timeout,
        Runout,
        Interrupted,
        InternalError,
    };
    RunningResult find_first_and_run(const std::vector<std::string>& list, std::chrono::milliseconds find_timeout,
                                     /*out*/ MAA_PIPELINE_RES_NS::TaskData& found_data);
    struct RecResult
    {
        cv::Rect box {};
    };
    struct FoundResult
    {
        RecResult rec;
        MAA_PIPELINE_RES_NS::TaskData task_data;
    };
    std::optional<FoundResult> find_first(const std::vector<std::string>& list);

    void start_to_act(const FoundResult& act);

private:
    std::optional<RecResult> recognize(const MAA_PIPELINE_RES_NS::TaskData& task_data);
    std::optional<RecResult> direct_hit(const cv::Mat& image, const MAA_VISION_NS::DirectHitParams& param,
                                        const cv::Rect& cache);
    std::optional<RecResult> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplMatchingParams& param,
                                            const cv::Rect& cache);
    std::optional<RecResult> ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParams& param,
                                             const cv::Rect& cache);

private:
    void click(const MAA_PIPELINE_RES_NS::Action::ClickParams& param, const cv::Rect& cur_box);
    void swipe(const MAA_PIPELINE_RES_NS::Action::SwipeParams& param, const cv::Rect& cur_box);
    void wait_freezes(const MAA_PIPELINE_RES_NS::Action::WaitFreezesParams& param, const cv::Rect& cur_box);

    cv::Rect get_target_rect(const MAA_PIPELINE_RES_NS::Action::Target type,
                             const MAA_PIPELINE_RES_NS::Action::TargetParam& param, const cv::Rect& cur_box);
};

MAA_TASK_NS_END
