#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Controller/ControllerAgent.h"
#include "Resource/ResourceMgr.h"
#include "RuntimeCache.h"
#include "Utils/MessageNotifier.hpp"

MAA_TASK_NS_BEGIN
class TaskBase;
MAA_TASK_NS_END

MAA_NS_BEGIN

class Tasker : public MaaTasker
{
public:
    Tasker(MaaNotificationCallback callback, void* callback_arg);
    virtual ~Tasker() override;

    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool bind_controller(MaaController* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_pipeline(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaTaskId post_recognition(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaTaskId post_action(const std::string& entry, const json::value& pipeline_override) override;
    virtual bool override_pipeline(MaaTaskId task_id, const json::value& pipeline_override) override;

    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;

    virtual MaaBool running() const override;
    virtual void post_stop() override;

    virtual MAA_RES_NS::ResourceMgr* resource() override;
    virtual MAA_CTRL_NS::ControllerAgent* controller() override;

public:
    RuntimeCache& runtime_cache();
    const RuntimeCache& runtime_cache() const;
    void notify(std::string_view msg, json::value detail = json::value());

private:
    using TaskPtr = std::shared_ptr<MAA_TASK_NS::TaskBase>;
    using RunnerId = AsyncRunner<TaskPtr>::Id;

    MaaTaskId post_task(TaskPtr task_ptr, const json::value& pipeline_override);
    bool run_task(RunnerId id, TaskPtr task_ptr);

    bool check_stop();
    RunnerId task_id_to_runner_id(MaaTaskId task_id) const;
    MaaTaskId runner_id_to_task_id(RunnerId runner_id) const;

private:
    MAA_RES_NS::ResourceMgr* resource_ = nullptr;
    MAA_CTRL_NS::ControllerAgent* controller_ = nullptr;

    bool need_to_stop_ = false;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;

    std::map<MaaTaskId, TaskPtr> task_cache_;
    std::map<MaaTaskId, RunnerId> task_id_mapping_;
    std::map<RunnerId, MaaTaskId> runner_id_mapping_;
    mutable std::mutex task_cache_mutex_;

    RuntimeCache runtime_cache_;

    MessageNotifier<MaaNotificationCallback> notifier;
};

MAA_NS_END
