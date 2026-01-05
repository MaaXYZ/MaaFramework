#pragma once

#include <atomic>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Context.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/RuntimeCache.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class TaskBase : public NonCopyable
{
public:
    TaskBase(std::string entry, Tasker* tasker, std::shared_ptr<Context> context = nullptr);
    virtual ~TaskBase() = default;

public:
    virtual bool run() { return true; }

    virtual void post_stop() {}

public:
    bool override_pipeline(const json::value& pipeline_override);

    Tasker* tasker() const;
    MaaTaskId task_id() const;
    const std::string& entry() const;

protected:
    MAA_RES_NS::ResourceMgr* resource();
    MAA_CTRL_NS::ControllerAgent* controller();

    RecoResult run_recognition(const cv::Mat& image, const PipelineData& data);
    ActionResult run_action(const RecoResult& reco, const PipelineData& data);
    cv::Mat screencap();
    MaaNodeId generate_node_id();
    void set_node_detail(MaaNodeId node_id, NodeDetail detail);
    void set_task_detail(TaskDetail detail);

    bool debug_mode() const;
    void notify(std::string_view msg, const json::value detail);

protected:
    const MaaTaskId task_id_ = ++s_global_task_id;
    Tasker* tasker_ = nullptr;

    const std::string entry_;
    std::string cur_node_;

    std::shared_ptr<Context> context_ = nullptr;

private:
    inline static std::atomic<MaaTaskId> s_global_task_id = kTaskIdBase;
    inline static std::atomic<MaaNodeId> s_global_node_id = kNodeIdBase;
};

MAA_TASK_NS_END
