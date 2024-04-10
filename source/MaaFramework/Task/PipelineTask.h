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

    static bool query_detail(MaaRunningId run_id, MaaRecoId& reco_id, bool& completed);

private:
    enum class RunningStatus
    {
        Success,
        Timeout,
        Runout,
        Interrupted,
        InternalError,
    };

    struct HitDetail
    {
        MaaRecoId reco_uid = 0;
        Recognizer::Hit reco_hit;
        json::value reco_detail;
        MAA_RES_NS::TaskData task_data;
    };

    struct RunningDetail
    {
        MaaRunningId run_id = 0;
        HitDetail hits;
        RunningStatus status = RunningStatus::InternalError;
    };

private:
    RunningStatus find_first_and_run(
        const std::vector<std::string>& list,
        std::chrono::milliseconds timeout,
        /*out*/ MAA_RES_NS::TaskData& found_data);
    std::optional<HitDetail> find_first(const std::vector<std::string>& list);
    RunningStatus run_task(const HitDetail& hits);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

    MAA_CTRL_NS::ControllerAgent* controller()
    {
        return inst_ ? inst_->inter_controller() : nullptr;
    }

    void notify(std::string_view msg, json::value detail = json::value())
    {
        if (inst_) {
            inst_->notify(msg, detail);
        }
    }

    bool need_to_stop() const { return need_to_stop_; }

    bool debug_mode() const;
    json::object basic_info();
    json::object reco_result_to_json(const std::string& name, const Recognizer::Result& res);
    json::object hit_detail_to_json(const HitDetail& detail);
    json::object running_detail_to_json(const RunningDetail& detail);

private:
    bool need_to_stop_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    int64_t task_id_ = 0;
    std::string entry_;
    std::string pre_hit_task_;

    std::map<std::string, uint64_t> run_times_map_;
    Actuator::PreTaskBoxes hit_cache_;

    TaskDataMgr data_mgr_;
};

MAA_TASK_NS_END
