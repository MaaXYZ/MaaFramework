#pragma once

#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>

#include <boost/asio/thread_pool.hpp>

#include "Base/AsyncRunner.hpp"
#include "Common/MaaTypes.h"
#include "Controller/ControllerAgent.h"
#include "Resource/ResourceMgr.h"
#include "RuntimeCache.h"
#include "Utils/EventDispatcher.hpp"

MAA_TASK_NS_BEGIN
class TaskBase;
MAA_TASK_NS_END

MAA_NS_BEGIN

class Tasker : public MaaTasker
{
public:
    Tasker();
    virtual ~Tasker() override;

    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool bind_controller(MaaController* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_task(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaTaskId post_recognition(const std::string& reco_type, const json::value& reco_param, const cv::Mat& image) override;
    virtual MaaTaskId
        post_action(const std::string& action_type, const json::value& action_param, const cv::Rect& box, const std::string& reco_detail)
            override;

    virtual bool override_pipeline(MaaTaskId task_id, const json::value& pipeline_override) override;

    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;

    virtual bool running() const override;
    virtual MaaTaskId post_stop() override;
    virtual bool stopping() const override;

    virtual MAA_RES_NS::ResourceMgr* resource() const override;
    virtual MAA_CTRL_NS::ControllerAgent* controller() const override;

    virtual void clear_cache() override;
    virtual std::optional<MAA_TASK_NS::TaskDetail> get_task_detail(MaaTaskId task_id) const override;
    virtual std::optional<MAA_TASK_NS::NodeDetail> get_node_detail(MaaNodeId node_id) const override;
    virtual std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId reco_id) const override;
    virtual std::optional<MAA_TASK_NS::ActionResult> get_action_result(MaaActId action_id) const override;
    virtual std::optional<MaaNodeId> get_latest_node(const std::string& node_name) const override;

    virtual MaaSinkId add_sink(MaaEventCallback callback, void* trans_arg) override;
    virtual void remove_sink(MaaSinkId sink_id) override;
    virtual void clear_sinks() override;

    virtual MaaSinkId add_context_sink(MaaEventCallback callback, void* trans_arg) override;
    virtual void remove_context_sink(MaaSinkId sink_id) override;
    virtual void clear_context_sinks() override;

public:
    RuntimeCache& runtime_cache();
    const RuntimeCache& runtime_cache() const;

    void context_notify(MaaContext* context, std::string_view msg, const json::value& details);

    boost::asio::thread_pool& reco_thread_pool();

private:
    using TaskPtr = std::shared_ptr<MAA_TASK_NS::TaskBase>;
    using RunnerId = AsyncRunner<TaskPtr>::Id;

    MaaTaskId post_task(TaskPtr task_ptr, const json::value& pipeline_override);
    bool run_task(RunnerId id, TaskPtr task_ptr);

    bool check_stop();
    RunnerId task_id_to_runner_id(MaaTaskId task_id) const;

private:
    MAA_RES_NS::ResourceMgr* resource_ = nullptr;
    MAA_CTRL_NS::ControllerAgent* controller_ = nullptr;

    bool need_to_stop_ = false;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    EventDispatcher notifier_;
    EventDispatcher context_notifier_;

    std::map<MaaTaskId, RunnerId> task_id_mapping_;
    mutable std::shared_mutex task_id_mapping_mutex_;

    RuntimeCache runtime_cache_;

    std::unique_ptr<boost::asio::thread_pool> reco_thread_pool_;
};

MAA_NS_END
