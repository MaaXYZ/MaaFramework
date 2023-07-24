#pragma once

#include <string_view>

#include <meojson/json.hpp>

#include "Common/MaaConf.h"
#include "Common/MaaTypes.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineConfig.h"
#include "Resource/PipelineTypes.h"

#include <stack>

MAA_TASK_NS_BEGIN

class PipelineTask : public MaaInstanceSink
{
public:
    PipelineTask(std::string first_task_name, InstanceInternalAPI* inst);
    virtual ~PipelineTask() override = default;

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

public:
    bool run();
    bool set_param(const json::value& param);
    std::string first_task_name() const { return first_task_name_; }

private:
    using TaskDataMap = MAA_RES_NS::PipelineConfig::TaskDataMap;

    enum class RunningResult
    {
        Success,
        Timeout,
        Runout,
        Interrupted,
        InternalError,
    };

    struct RecResult
    {
        cv::Rect box {};
    };

    struct FoundResult
    {
        RecResult rec;
        MAA_PIPELINE_RES_NS::TaskData task_data;
    };

private:
    bool set_diff_task(const json::value& input);
    bool check_and_load_template_images(TaskDataMap& map);

private:
    RunningResult find_first_and_run(const std::vector<std::string>& list, std::chrono::milliseconds find_timeout,
                                     /*out*/ MAA_PIPELINE_RES_NS::TaskData& found_data);
    std::optional<FoundResult> find_first(const std::vector<std::string>& list);
    void start_to_act(const FoundResult& act);

private:
    std::optional<RecResult> recognize(const cv::Mat& image, const MAA_PIPELINE_RES_NS::TaskData& task_data);
    std::optional<RecResult> direct_hit(const cv::Mat& image, const MAA_VISION_NS::DirectHitParam& param,
                                        const cv::Rect& cache);
    std::optional<RecResult> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplMatchingParam& param,
                                            const cv::Rect& cache);
    std::optional<RecResult> ocr(const cv::Mat& image, const MAA_VISION_NS::OcrParam& param, const cv::Rect& cache);
    std::optional<RecResult> custom_recognize(const cv::Mat& image, const MAA_VISION_NS::CustomParam& param,
                                              const cv::Rect& cache);

private:
    void click(const MAA_PIPELINE_RES_NS::Action::ClickParam& param, const cv::Rect& cur_box);
    void swipe(const MAA_PIPELINE_RES_NS::Action::SwipeParam& param, const cv::Rect& cur_box);
    void press_key(const MAA_PIPELINE_RES_NS::Action::KeyParam& param);

    void start_app(const MAA_PIPELINE_RES_NS::Action::AppParam& param);
    void stop_app(const MAA_PIPELINE_RES_NS::Action::AppParam& param);
    void custom_action(const MAA_PIPELINE_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box);

    void wait_freezes(const MAA_PIPELINE_RES_NS::WaitFreezesParam& param, const cv::Rect& cur_box);

    cv::Rect get_target_rect(const MAA_PIPELINE_RES_NS::Action::Target target, const cv::Rect& cur_box);

    const MAA_PIPELINE_RES_NS::TaskData& get_task_data(const std::string& task_name);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }
    MAA_CTRL_NS::ControllerMgr* controller() { return inst_ ? inst_->inter_controller() : nullptr; }
    InstanceStatus* status() { return inst_ ? inst_->status() : nullptr; }

    bool need_exit() const { return need_exit_; }
    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;

private:
    bool need_exit_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    std::string first_task_name_;
    std::string cur_task_name_;
    TaskDataMap diff_tasks_;
};

MAA_TASK_NS_END
