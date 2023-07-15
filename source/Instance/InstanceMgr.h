#pragma once

#include "Base/AsyncRunner.hpp"
#include "Base/MessageNotifier.hpp"
#include "Common/MaaTypes.h"
#include "Instance/InstanceStatus.h"
#include "InstanceInternalAPI.hpp"
#include "Task/TaskBase.h"

#include <mutex>

MAA_NS_BEGIN

class InstanceMgr : public MaaInstanceAPI, public InstanceInternalAPI
{
public:
    InstanceMgr(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    virtual ~InstanceMgr() override;

public: // from MaaInstanceAPI
    virtual bool bind_resource(MaaResourceAPI* resource) override;
    virtual bool bind_controller(MaaControllerAPI* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_task(std::string task, std::string_view param) override;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) override;
    virtual void register_custom_task(std::string name, MaaCustomTaskHandle handle) override;
    virtual void unregister_custom_task(std::string name) override;
    virtual void clear_custom_task() override;

    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;
    virtual MaaBool all_finished() const override;

    virtual void stop() override;

    virtual MaaResourceHandle resource() override;
    virtual MaaControllerHandle controller() override;

public: // from InstanceInternalAPI
    virtual MAA_RES_NS::ResourceMgr* inter_resource() override;
    virtual MAA_CTRL_NS::ControllerMgr* inter_controller() override;
    virtual InstanceStatus* status() override;
    virtual MAA_TASK_NS::CustomTaskPtr custom_task(const std::string& name) override;

private:
    using TaskPtr = std::shared_ptr<TaskNS::TaskBase>;
    using TaskId = AsyncRunner<TaskPtr>::Id;

    bool run_task(TaskId id, TaskPtr task_ptr);

private:
    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;
    InstanceStatus status_;

    std::map<std::string, MAA_TASK_NS::CustomTaskPtr> custom_tasks_;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    MessageNotifier<MaaInstanceCallback> notifier;
};

MAA_NS_END
