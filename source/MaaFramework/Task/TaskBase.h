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
    TaskBase(std::string entry, Tasker* tasker);
    TaskBase(std::string entry, Tasker* tasker, std::shared_ptr<Context> context);
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
    NodeDetail run_action(const RecoResult& reco);
    cv::Mat screencap();
    MaaTaskId generate_node_id();
    void set_node_detail(int64_t node_id, NodeDetail detail);
    void set_task_detail(TaskDetail detail);

private:
    void init();
    bool debug_mode() const;
    json::object basic_info();
    static json::object reco_detail_to_json(const RecoResult& res);
    static json::object node_detail_to_json(const NodeDetail& detail);

protected:
    const MaaTaskId task_id_ = ++s_global_task_id;
    Tasker* tasker_ = nullptr;

    const std::string entry_;
    std::string cur_task_;

    std::shared_ptr<Context> context_ = nullptr;

private:
    inline static std::atomic<MaaTaskId> s_global_task_id = 100000000;
    inline static std::atomic<MaaTaskId> s_global_node_id = 200000000;
};

MAA_TASK_NS_END
