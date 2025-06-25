#pragma once

#include <atomic>

#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
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
    TaskBase(std::string entry, Tasker* tasker);
    TaskBase(std::string entry, Tasker* tasker, std::shared_ptr<Context> context);
    virtual ~TaskBase() = default;

    virtual bool run() = 0;
    virtual void post_stop() = 0;

public:
    bool override_pipeline(const json::object& pipeline_override);

public:
    Tasker* tasker() const;
    MaaTaskId task_id() const;
    const std::string& entry() const;

protected:
    MAA_RES_NS::ResourceMgr* resource();
    MAA_CTRL_NS::ControllerAgent* controller();

    RecoResult run_recognition(const cv::Mat& image, const PipelineData::NextList& list);
    NodeDetail run_action(const RecoResult& reco);
    cv::Mat screencap();
    MaaTaskId generate_node_id();
    void set_node_detail(int64_t node_id, NodeDetail detail);
    void set_task_detail(TaskDetail detail);

protected:
    const MaaTaskId task_id_ = ++s_global_task_id;
    Tasker* tasker_ = nullptr;

    const std::string entry_;
    std::string cur_node_;

    std::shared_ptr<Context> context_ = nullptr;

private:
    bool debug_mode() const;
    void notify(std::string_view msg, const json::value detail);

private:
    inline static std::atomic<MaaTaskId> s_global_task_id = 100'000'000;
    inline static std::atomic<MaaTaskId> s_global_node_id = 200'000'000;
};

MAA_TASK_NS_END
