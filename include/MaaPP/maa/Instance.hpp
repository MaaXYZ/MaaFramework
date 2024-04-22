#pragma once

#include <iostream>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaFramework/Instance/MaaInstance.h"
#include "MaaFramework/MaaDef.h"
#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Resource.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"
#include "meojson/common/utils.hpp"

namespace maa
{

class Instance;

class InstanceAction : public ActionBase<InstanceAction, Instance, MaaTaskId>
{
    friend class Instance;

public:
    using ActionBase::ActionBase;

    MaaStatus status();

    coro::Promise<MaaStatus> wait() { return status_; }

private:
    coro::Promise<MaaStatus> status_;
};

class Instance : public ActionHelper<Instance, InstanceAction, MaaTaskId, MaaInstanceHandle>
{
    friend class InstanceAction;

public:
    Instance()
        : ActionHelper(MaaCreate(&Instance::_callback, this))
    {
    }

    ~Instance() { MaaDestroy(inst_); }

    std::shared_ptr<InstanceAction>
        post_task(const std::string& task, const json::object& param = json::object {})
    {
        return put_action(MaaPostTask(inst_, task.c_str(), param.to_string().c_str()));
    }

    bool bind(std::shared_ptr<Controller> ctrl)
    {
        if (!MaaBindController(inst_, ctrl ? ctrl->inst_ : nullptr)) {
            return false;
        }
        controller_ = ctrl;
        return true;
    }

    bool bind(std::shared_ptr<Resource> res)
    {
        if (!MaaBindResource(inst_, res ? res->inst_ : nullptr)) {
            return false;
        }
        resource_ = res;
        return true;
    }

    bool inited() { return MaaInited(inst_); }

private:
    static void _callback(MaaStringView msg, MaaStringView details, MaaTransparentArg arg)
    {
        auto detail_opt = json::parse(details);
        if (!detail_opt.has_value() || !detail_opt.value().is_object()) {
            return;
        }
        // prevent destroy
        auto self = reinterpret_cast<Instance*>(arg)->shared_from_this();

        coro::EventLoop::current()->defer(
            [self, msg_str = std::string(msg), detail_val = std::move(detail_opt.value())]() {
                const auto& detail_obj = detail_val.as_object();
                if (!detail_obj.contains("id")) {
                    return;
                }
                MaaTaskId id = detail_obj.at("id").as_unsigned_long_long();
                if (!self->actions_.contains(id)) {
                    std::cout << "cannot find id " << id << std::endl;
                    return;
                }
                auto ptr = self->actions_.at(id).lock();
                if (!ptr) {
                    std::cout << "action id " << id << " expired" << std::endl;
                    return;
                }
                if (msg_str == MaaMsg_Task_Completed) {
                    ptr->status_.resolve(MaaStatus_Success);
                }
                else if (msg_str == MaaMsg_Task_Failed) {
                    ptr->status_.resolve(MaaStatus_Failed);
                }
            });
    }

    std::shared_ptr<Controller> controller_;
    std::shared_ptr<Resource> resource_;
};

inline MaaStatus InstanceAction::status()
{
    return MaaTaskStatus(inst_->inst_, id_);
}

}
