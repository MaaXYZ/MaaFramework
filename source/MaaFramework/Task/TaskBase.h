#pragma once

#include <atomic>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Context.h"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/RuntimeCache.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class TaskBase
{
public:
    using PipelineData = Context::PipelineData;
    using PipelineDataMap = Context::PipelineDataMap;
    using NextIter = PipelineData::NextList::const_iterator;

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

protected:
    MAA_RES_NS::ResourceMgr* resource();
    MAA_CTRL_NS::ControllerAgent* controller();
    void notify(std::string_view msg, json::value detail = json::value());

    NextIter run_recogintion(const cv::Mat& image, const PipelineData::NextList& list, HitDetail& hit_detail);
    bool run_action(const HitDetail& hit);

private:
    void add_node_detail(int64_t node_id, NodeDetail detail);

    bool debug_mode() const;
    json::object basic_info();
    static json::object reco_result_to_json(const std::string& name, const RecoResult& res);
    static json::object hit_detail_to_json(const HitDetail& detail);
    static json::object node_detail_to_json(const NodeDetail& detail);

protected:
    Tasker* tasker_ = nullptr;
    Context context_;

    const int64_t task_id_ = ++s_global_task_id;

    std::string entry_;
    std::string pre_hit_task_;

    std::map<std::string, uint64_t> hit_times_map_;

    inline static std::atomic<MaaNodeId> s_global_task_id = 0;
};

MAA_TASK_NS_END
