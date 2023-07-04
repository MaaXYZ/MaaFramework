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

    virtual bool set_option(MaaInstOption key, const std::string& value) override;

    virtual MaaTaskId post_task(std::string_view task, std::string_view param) override;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) override;
    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;
    virtual MaaBool all_finished() const override;

    virtual void stop() override;

    virtual std::string get_resource_hash() const override;
    virtual std::string get_controller_uuid() const override;

public: // from InstanceInternalAPI
    virtual MAA_RES_NS::ResourceMgr* resource() override;
    virtual MAA_CTRL_NS::ControllerMgr* controller() override;
    virtual InstanceStatus* status() override;

private:
    using TaskPtr = std::shared_ptr<TaskNS::TaskBase>;
    using TaskId = AsyncRunner<TaskPtr>::Id;

    bool run_task(TaskId id, TaskPtr task_ptr);

private:
    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;
    InstanceStatus status_;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    MessageNotifier<MaaInstanceCallback> notifier;
};

MAA_NS_END
