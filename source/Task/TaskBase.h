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
    TaskBase(std::string_view task_name, InstanceInternalAPI* inst) : task_name_(task_name), inst_(inst) {}
    virtual ~TaskBase() = default;

    virtual bool run() = 0;
    virtual bool set_param(const json::value& param) = 0;

    virtual std::string_view type() const = 0;

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

protected:
    MAA_RES_NS::ResourceMgr* resource() { return inst_->resource(); }
    MAA_CTRL_NS::ControllerMgr* controller() { return inst_->controller(); }
    InstanceStatus* status() { return inst_->status(); }

protected:
    bool need_exit() const { return need_exit_; }
    void sleep(uint ms) const;
    InstanceInternalAPI* inst() const { return inst_; }

    std::string_view task_name_;

private:
    bool need_exit_ = false;
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
