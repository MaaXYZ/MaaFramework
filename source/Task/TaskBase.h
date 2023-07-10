#pragma once

#include "Common/MaaConf.h"

#include <string_view>

#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
#include "Instance/InstanceInternalAPI.hpp"

MAA_TASK_NS_BEGIN

class TaskBase : public MaaInstanceSink
{
public:
    TaskBase(std::string task_name, InstanceInternalAPI* inst) : first_task_(std::move(task_name)), inst_(inst) {}
    virtual ~TaskBase() = default;

    virtual bool run() = 0;
    virtual bool set_param(const json::value& param) = 0;

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

protected:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->resource() : nullptr; }
    MAA_CTRL_NS::ControllerMgr* controller() { return inst_ ? inst_->controller() : nullptr; }
    InstanceStatus* status() { return inst_ ? inst_->status() : nullptr; }

protected:
    bool need_exit() const { return need_exit_; }
    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;
    InstanceInternalAPI* inst() const { return inst_; }

    std::string first_task_;

private:
    bool need_exit_ = false;
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
