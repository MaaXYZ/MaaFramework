#pragma once

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

#include <stack>

MAA_TASK_NS_BEGIN

class PipelineTask : public MaaInstanceSink
{
public:
    using TaskData = MAA_RES_NS::TaskData;

public:
    PipelineTask(std::string entry, InstanceInternalAPI* inst);
    virtual ~PipelineTask() override = default;

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

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
        Recognizer::Result rec_result;
        MAA_RES_NS::TaskData task_data;
    };

private:
    RunningResult find_first_and_run(const std::vector<std::string>& list, std::chrono::milliseconds timeout,
                                     /*out*/ MAA_RES_NS::TaskData& found_data);
    std::optional<HitResult> find_first(const std::vector<std::string>& list);
    RunningResult run_task(const HitResult& hits);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }
    MAA_CTRL_NS::ControllerAgent* controller() { return inst_ ? inst_->inter_controller() : nullptr; }
    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }
    void notify(std::string_view msg, json::value detail = json::value())
    {
        if (inst_) inst_->notify(msg, detail);
    }

    bool need_exit() const { return need_exit_; }

private:
    bool need_exit_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    int64_t task_id_ = 0;
    std::string entry_;
    std::string cur_task_name_;

    TaskDataMgr data_mgr_;
    Recognizer recognizer_;
    Actuator actuator_;
};

MAA_TASK_NS_END
