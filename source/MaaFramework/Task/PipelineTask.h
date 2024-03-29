#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Task/Actuator.h"
#include "Task/Recognizer.h"
#include "Task/TaskDataMgr.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public MaaInstanceSink
{
public:
    using TaskData = MAA_RES_NS::TaskData;

public:
    PipelineTask(std::string entry, InstanceInternalAPI* inst);
    virtual ~PipelineTask() override = default;

public: // from MaaInstanceSink
    virtual void post_stop() override { need_to_stop_ = true; }

public:
    const std::string& entry() const { return entry_; }

    bool run();

    void set_taskid(int64_t id) { task_id_ = id; }

    bool set_param(const json::value& param);

private:
    enum class RunningResult
    {
        Success,
        Timeout,
        Runout,
        Interrupted,
        InternalError,
    };

    struct HitResult
    {
        Recognizer::Hit reco_hit;
        MAA_RES_NS::TaskData task_data;
    };

private:
    RunningResult find_first_and_run(
        const std::vector<std::string>& list,
        std::chrono::milliseconds timeout,
        /*out*/ MAA_RES_NS::TaskData& found_data);
    std::optional<HitResult> find_first(const std::vector<std::string>& list);
    RunningResult run_task(const HitResult& hits);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

    MAA_CTRL_NS::ControllerAgent* controller()
    {
        return inst_ ? inst_->inter_controller() : nullptr;
    }

    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }

    void notify(std::string_view msg, json::value detail = json::value())
    {
        if (inst_) {
            inst_->notify(msg, detail);
        }
    }

    bool need_to_stop() const { return need_to_stop_; }

    bool debug_mode() const;
    json::object basic_info();
    std::filesystem::path dump_image(const cv::Mat& image) const;

private:
    bool need_to_stop_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    int64_t task_id_ = 0;
    std::string entry_;
    std::string latest_hit_;

    TaskDataMgr data_mgr_;
    Recognizer recognizer_;
    Actuator actuator_;
};

MAA_TASK_NS_END
