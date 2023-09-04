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
#include "Task/TaskInstAPI.h"

#include <stack>

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskInstAPI, public MaaInstanceSink
{
public:
    using TaskData = MAA_RES_NS::TaskData;

public:
    PipelineTask(std::string entry, InstanceInternalAPI* inst);
    PipelineTask(std::string entry, TaskInstAPI& task_inst);
    virtual ~PipelineTask() override = default;

public: // from TaskInstAPI
    virtual InstanceInternalAPI* inst() override { return inst_; }
    virtual TaskDataMgr& data_mgr() { return data_mgr_; }

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

public:
    const std::string& entry() const { return entry_; }

    bool run();
    bool set_param(const json::value& param);

    Recognizer& recognizer() { return recognizer_; }
    Actuator& actuator() { return actuator_; }

private:
    enum class RunningResult
    {
        Success,
        Timeout,
        Runout,
        Interrupted,
        InternalError,
    };

    struct RecognitionResult
    {
        Recognizer::Result rec_result;
        MAA_RES_NS::TaskData task_data;
    };

private:
    RunningResult find_first_and_run(const std::vector<std::string>& list, std::chrono::milliseconds find_timeout,
                                     /*out*/ MAA_RES_NS::TaskData& found_data);
    std::optional<RecognitionResult> find_first(const std::vector<std::string>& list);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }
    MAA_CTRL_NS::ControllerMgr* controller() { return inst_ ? inst_->inter_controller() : nullptr; }
    InstanceStatus* status() { return inst_ ? inst_->inter_status() : nullptr; }

    bool need_exit() const { return need_exit_; }

private:
    bool need_exit_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    std::string entry_;
    std::string cur_task_name_;

    TaskDataMgr data_mgr_;
    Recognizer recognizer_;
    Actuator actuator_;
};

MAA_TASK_NS_END
