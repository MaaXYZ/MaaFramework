#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"
#include "Task/AbstractTask.h"
#include "Utils/AsyncRunner.hpp"

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

    virtual MaaTaskId append_task(std::string_view type, std::string_view param) override;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) override;
    virtual std::vector<MaaTaskId> get_task_list() const override;

    virtual void stop() override;
    virtual bool running() const override;

    virtual std::string get_resource_hash() const override;
    virtual std::string get_controller_uuid() const override;

protected:
    using TaskPtr = std::shared_ptr<TaskNS::AbstractTask>;

    struct NotifyData
    {
        MaaMsg msg = MaaMsg::InvalidMsg;
        json::value details;
    };

    void run_task(typename AsyncRunner<TaskPtr>::Id id, TaskPtr task_ptr);
    void notify(typename AsyncRunner<NotifyData>::Id id, NotifyData cb_data);

protected:
    MaaInstanceCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;

    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    std::unique_ptr<AsyncRunner<NotifyData>> notify_runner_ = nullptr;

    std::map<typename AsyncRunner<TaskPtr>::Id, TaskPtr> task_map_;
};

MAA_NS_END
