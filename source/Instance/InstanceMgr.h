#pragma once

#include "Base/AsyncCallback.hpp"
#include "Base/AsyncRunner.hpp"
#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"
#include "Task/AbstractTask.h"

#include <mutex>

MAA_NS_BEGIN

class InstanceMgr : public MaaInstanceAPI
{
public:
    InstanceMgr(MaaInstanceCallback callback, void* callback_arg);
    virtual ~InstanceMgr() override;

    virtual bool bind_resource(MaaResourceAPI* resource) override;
    virtual bool bind_controller(MaaControllerAPI* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(std::string_view key, std::string_view value) override;

    virtual MaaTaskId post_task(std::string_view type, std::string_view param) override;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) override;
    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaBool all_finished() const override;

    virtual void stop() override;

    virtual std::string get_resource_hash() const override;
    virtual std::string get_controller_uuid() const override;

private:
    using TaskPtr = std::shared_ptr<TaskNS::AbstractTask>;

    bool run_task(typename AsyncRunner<TaskPtr>::Id id, TaskPtr task_ptr);

private:
    AsyncCallback<MaaInstanceCallback, void*> notifier;

    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    std::map<typename AsyncRunner<TaskPtr>::Id, TaskPtr> task_map_;
    std::mutex task_mutex_;
};

MAA_NS_END
