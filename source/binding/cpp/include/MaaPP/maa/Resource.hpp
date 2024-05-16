// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <iostream>
#include <ranges>
#include <utility>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Exception.hpp"
#include "MaaPP/maa/Message.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"
#include "MaaPP/maa/details/Message.hpp"
#include "MaaPP/maa/details/String.hpp"

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
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<Resource>(std::forward<Args>(args)...);
    }

    Resource(std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : ActionHelper(MaaResourceCreate(&Resource::_callback, this))
        , user_callback_(callback)
    {
    }

    ~Resource()
    {
        MaaResourceDestroy(inst_);
        for (auto action : actions_ | std::views::values) {
            action.lock()->wait().sync_wait();
        }
    }

    std::shared_ptr<ResourceAction> post_path(const std::string& path)
    {
        return put_action(MaaResourcePostPath(inst_, path.c_str()));
    }

    std::shared_ptr<Resource> clear()
    {
        if (!MaaResourceClear(inst_)) {
            throw FunctionFailed("MaaResourceClear");
        }
        return shared_from_this();
    }

    bool loaded() { return MaaResourceLoaded(inst_); }

    std::string hash()
    {
        details::String buf;
        if (!MaaResourceGetHash(inst_, buf.handle())) {
            throw FunctionFailed("MaaResourceGetHash");
        }
        return buf;
    }

    std::shared_ptr<std::vector<std::string>> task_list()
    {
        details::String buf;
        if (!MaaResourceGetTaskList(inst_, buf.handle())) {
            throw FunctionFailed("MaaResourceGetTaskList");
        }
        return std::make_shared<std::vector<std::string>>(
            json::parse(buf.str()).value().as<std::vector<std::string>>());
    }

private:
    static void _callback(MaaStringView msg, MaaStringView details, MaaTransparentArg arg)
    {
        auto msg_ptr = message::parse(msg, details);

        auto self = reinterpret_cast<Resource*>(arg)->shared_from_this();

        coro::EventLoop::current()->defer([self, msg_ptr]() {
            if (auto msg = msg_ptr->is<message::ResourceLoadingMessage>()) {
                auto id = msg->id;
                if (!self->actions_.contains(id)) {
                    std::cout << "cannot find id " << id << std::endl;
                    return;
                }
                auto ptr = self->actions_.at(id).lock();
                if (!ptr) {
                    std::cout << "action id " << id << " expired" << std::endl;
                    return;
                }
                if (msg->type == message::ResourceLoadingMessage::Completed) {
                    ptr->status_.resolve(MaaStatus_Success);
                }
                else if (msg->type == message::ResourceLoadingMessage::Failed) {
                    ptr->status_.resolve(MaaStatus_Failed);
                }
            }
        });

        if (self->user_callback_) {
            coro::EventLoop::current()->defer([self, msg_ptr]() { self->user_callback_(msg_ptr); });
        }
    }

    std::function<void(std::shared_ptr<message::MessageBase>)> user_callback_;
};

inline MaaStatus ResourceAction::status()
{
    return MaaResourceStatus(inst_->inst_, id_);
}

}
