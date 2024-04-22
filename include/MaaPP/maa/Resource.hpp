#pragma once

#include <iostream>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"

namespace maa
{

class Resource;

class ResourceAction : public details::ActionBase<ResourceAction, Resource>
{
    friend class Resource;

public:
    using ActionBase::ActionBase;

    MaaStatus status();

    coro::Promise<MaaStatus> wait() { return status_; }

private:
    coro::Promise<MaaStatus> status_;
};

class Resource : public details::ActionHelper<Resource, ResourceAction, MaaResourceHandle>
{
    friend class ResourceAction;
    friend class Instance;

public:
    Resource()
        : ActionHelper(MaaResourceCreate(&Resource::_callback, this))
    {
    }

    ~Resource() { MaaResourceDestroy(inst_); }

    std::shared_ptr<ResourceAction> post_path(const std::string& path)
    {
        return put_action(MaaResourcePostPath(inst_, path.c_str()));
    }

private:
    static void _callback(MaaStringView msg, MaaStringView details, MaaTransparentArg arg)
    {
        auto detail_opt = json::parse(details);
        if (!detail_opt.has_value() || !detail_opt.value().is_object()) {
            return;
        }
        // prevent destroy
        auto self = reinterpret_cast<Resource*>(arg)->shared_from_this();

        coro::EventLoop::current()->defer(
            [self, msg_str = std::string(msg), detail_val = std::move(detail_opt.value())]() {
                const auto& detail_obj = detail_val.as_object();
                if (!detail_obj.contains("id")) {
                    return;
                }
                MaaResId id = detail_obj.at("id").as_unsigned_long_long();
                if (!self->actions_.contains(id)) {
                    std::cout << "cannot find id " << id << std::endl;
                    return;
                }
                auto ptr = self->actions_.at(id).lock();
                if (!ptr) {
                    std::cout << "action id " << id << " expired" << std::endl;
                    return;
                }
                if (msg_str == MaaMsg_Resource_LoadingCompleted) {
                    ptr->status_.resolve(MaaStatus_Success);
                }
                else if (msg_str == MaaMsg_Resource_LoadingFailed) {
                    ptr->status_.resolve(MaaStatus_Failed);
                }
            });
    }
};

inline MaaStatus ResourceAction::status()
{
    return MaaResourceStatus(inst_->inst_, id_);
}

}
