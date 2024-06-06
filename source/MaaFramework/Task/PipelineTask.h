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

    // TODO: 重构，拆分成三个单独的类
    enum RunType
    {
        Pipeline,
        Recognition,
        Action,
    };

    void set_type(RunType type) { run_type_ = type; }

    bool set_param(const json::value& param);

    static bool query_node_detail(
        MaaNodeId node_id,
        std::string& name,
        MaaRecoId& reco_id,
        bool& completed);

    static bool query_task_detail(
        MaaTaskId task_id,
        std::string& entry,
        std::vector<MaaNodeId>& node_id_list);

private:
    enum class NodeStatus
    {
        None,
        RunCompleted,
        OnlyRecognized,
    };

    struct HitDetail
    {
        MaaRecoId reco_uid = 0;
        Recognizer::Hit reco_hit {};
        json::value reco_detail;
        MAA_RES_NS::TaskData task_data;
    };

    struct NodeDetail
    {
        const MaaNodeId node_id = ++s_global_node_id;

        std::string name;
        HitDetail hit;
        NodeStatus status = NodeStatus::None;
    };

    struct TaskDetail
    {
        std::string entry;
        std::vector<MaaNodeId> node_ids;
    };

private:
    // TODO: 重构，拆分成三个单独的类
    bool run_pipeline();
    bool run_recognition_only();
    bool run_action_only();

    TaskData::NextList::const_iterator find_first_and_run(const TaskData::NextList& list);
    TaskData::NextList::const_iterator
        find_first(const TaskData::NextList& list, HitDetail& hit_detail);
    bool run_task(const HitDetail& hit);

    void add_node_detail(int64_t node_id, NodeDetail detail);

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
    json::object node_detail_to_json(const NodeDetail& detail);

private:
    RunType run_type_ = RunType::Pipeline;
    bool need_to_stop_ = false;
    InstanceInternalAPI* inst_ = nullptr;

    int64_t task_id_ = 0;
    std::string entry_;
    std::string pre_hit_task_;

    std::map<std::string, uint64_t> hit_times_map_;

    TaskDataMgr data_mgr_;

    inline static std::atomic<MaaNodeId> s_global_node_id = 0;
};

MAA_TASK_NS_END
