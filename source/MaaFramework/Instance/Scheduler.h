#pragma once

#include <mutex>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Controller/ControllerAgent.h"
#include "Resource/ResourceMgr.h"
#include "Task/PipelineTask.h"
#include "TaskCache.h"
#include "Utils/Dispatcher.hpp"
#include "Utils/MessageNotifier.hpp"

MAA_NS_BEGIN

class Scheduler : public MaaScheduler
{
public:
    Scheduler(MaaNotificationCallback callback, MaaTransparentArg callback_arg);
    virtual ~Scheduler() override;

    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool bind_controller(MaaController* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(MaaSchedOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_pipeline(std::string entry, std::string_view param) override;
    virtual MaaTaskId post_recognition(std::string entry, std::string_view param) override;
    virtual MaaTaskId post_action(std::string entry, std::string_view param) override;
    virtual bool set_param(MaaTaskId task_id, std::string_view param) override;

    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;

    virtual MaaBool running() const override;
    virtual void post_stop() override;

    virtual MAA_RES_NS::ResourceMgr* resource() override;
    virtual MAA_CTRL_NS::ControllerAgent* controller() override;

public:
    TaskCache& cache();
    const TaskCache& cache() const;

private:
    using TaskPtr = std::shared_ptr<TaskNS::PipelineTask>;
    using TaskId = AsyncRunner<TaskPtr>::Id;

    TaskPtr make_task(std::string entry, std::string_view param);
    TaskId post_task(const TaskPtr& task_ptr);
    bool run_task(TaskId id, TaskPtr task_ptr);
    bool check_stop();

private:
    MAA_RES_NS::ResourceMgr* resource_ = nullptr;
    MAA_CTRL_NS::ControllerAgent* controller_ = nullptr;

    bool need_to_stop_ = false;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    std::map<TaskId, TaskPtr> task_cache_;
    std::mutex task_cache_mutex_;

    TaskCache cache_;

    MessageNotifier<MaaNotificationCallback> notifier;
};

MAA_NS_END
