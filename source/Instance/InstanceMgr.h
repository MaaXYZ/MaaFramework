#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

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

    virtual MaaTaskId append_task(std::string_view type, const std::string& param) override;
    virtual bool set_task_param(MaaTaskId task_id, const std::string& param) override;
    virtual std::vector<MaaTaskId> get_task_list() const override;

    virtual bool start() override;
    virtual bool stop() override;
    virtual bool running() const override;

    virtual std::string get_resource_hash() const override;
    virtual std::string get_controller_uuid() const override;

protected:
    MaaInstanceCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;

    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;
};

MAA_NS_END
