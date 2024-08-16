#pragma once

#include <atomic>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Context.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/RuntimeCache.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class TaskBase
{
public:
    using PipelineData = Context::PipelineData;
    using PipelineDataMap = Context::PipelineDataMap;

public:
    TaskBase(std::string entry, Tasker* tasker, PipelineDataMap pp_override);
    virtual ~TaskBase() = default;

    virtual bool run() = 0;
    virtual void post_stop() = 0;

public:
    bool override_pipeline(const json::value& pipeline_override);

public:
    Tasker* tasker() const;
    MaaTaskId task_id() const;
    const std::string& entry() const;

protected:
    MAA_RES_NS::ResourceMgr* resource();
    MAA_CTRL_NS::ControllerAgent* controller();
    void notify(std::string_view msg, json::value detail = json::value());

    RecoResult run_recogintion(const cv::Mat& image, const PipelineData::NextList& list);
    bool run_action(const RecoResult& reco);
    cv::Mat screencap();
    void add_node_detail(int64_t node_id, NodeDetail detail);

private:
    bool debug_mode() const;
    json::object basic_info();
    static json::object reco_result_to_json(const std::string& name, const RecoResult& res);
    static json::object node_detail_to_json(MaaNodeId node_id, const NodeDetail& detail);

protected:
    Tasker* tasker_ = nullptr;
    Context context_;

    const MaaTaskId task_id_ = ++s_global_task_id;

    const std::string entry_;
    std::string pre_hit_task_;

    std::map<std::string, uint64_t> hit_times_map_;

    inline static std::atomic<MaaTaskId> s_global_task_id = 0;
    inline static std::atomic<MaaTaskId> s_global_node_id = 0;
};

MAA_TASK_NS_END
