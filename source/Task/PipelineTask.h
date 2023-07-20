#pragma once

#include "Resource/PipelineConfig.h"
#include "Resource/PipelineTypes.h"
#include "TaskBase.h"

#include <stack>

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;
    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual bool set_param(const json::value& param) override;

private:
    using TaskDataMap = MAA_RES_NS::PipelineConfig::TaskDataMap;

    bool set_diff_task(const json::value& input);
    bool check_and_load_template_images(TaskDataMap& map);

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

    bool run_all(const std::vector<std::string>& list);

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
    std::optional<RecResult> recognize(const cv::Mat& image, const MAA_PIPELINE_RES_NS::TaskData& task_data);
    std::optional<RecResult> direct_hit(const cv::Mat& image, const MAA_VISION_NS::DirectHitParams& param,
                                        const cv::Rect& cache);
    std::optional<RecResult> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplMatchingParams& param,
                                            const cv::Rect& cache);
    std::optional<RecResult> ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParams& param, const cv::Rect& cache);

private:
    void click(const MAA_PIPELINE_RES_NS::Action::ClickParams& param, const cv::Rect& cur_box);
    void swipe(const MAA_PIPELINE_RES_NS::Action::SwipeParams& param, const cv::Rect& cur_box);
    void press_key(const MAA_PIPELINE_RES_NS::Action::KeyParams& param);

    void start_app(const MAA_PIPELINE_RES_NS::Action::AppInfo& param);
    void stop_app(const MAA_PIPELINE_RES_NS::Action::AppInfo& param);
    void run_custom_task(const MAA_PIPELINE_RES_NS::Action::CustomTaskParams& param);

    void wait_freezes(const MAA_PIPELINE_RES_NS::WaitFreezesParams& param, const cv::Rect& cur_box);

    cv::Rect get_target_rect(const MAA_PIPELINE_RES_NS::Action::Target type,
                             const MAA_PIPELINE_RES_NS::Action::TargetParam& param, const cv::Rect& cur_box);

    const MAA_PIPELINE_RES_NS::TaskData& get_task_data(const std::string& task_name);

private:
    std::stack<std::string> breakpoints_;
    TaskDataMap diff_tasks_;
};

MAA_TASK_NS_END
