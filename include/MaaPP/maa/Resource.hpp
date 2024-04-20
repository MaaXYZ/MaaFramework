#pragma once

#include <iostream>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"

namespace maa
{

class Resource : public std::enable_shared_from_this<Resource>
{
public:
    class Action : public std::enable_shared_from_this<Action>
    {
    public:
        friend class Resource;

        Action(std::shared_ptr<Resource> res, MaaResId id)
            : res_(res)
            , id_(id)
        {
        }

        Action(const Action&) = delete;
        Action& operator=(const Action&) = delete;

        ~Action() { res_->actions_.erase(id_); }

        MaaStatus status() { return MaaResourceStatus(res_->res_, id_); }

        coro::Promise<MaaStatus> wait() { return status_; }

    private:
        std::shared_ptr<Resource> res_;
        MaaResId id_;
        coro::Promise<MaaStatus> status_;
    };

    Resource()
    {
        res_ = MaaResourceCreate(
            [](MaaStringView msg, MaaStringView details, MaaTransparentArg arg) {
                auto detail_opt = json::parse(details);
                if (!detail_opt.has_value() || !detail_opt.value().is_object()) {
                    return;
                }
                // prevent destroy
                auto self = reinterpret_cast<Resource*>(arg)->shared_from_this();

                coro::EventLoop::current()->defer([self,
                                                   msg_str = std::string(msg),
                                                   detail_val = std::move(detail_opt.value())]() {
                    const auto& detail_obj = detail_val.as_object();
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
                    if (msg_str == MaaMsg_Resource_StartLoading) {
                    }
                    else if (msg_str == MaaMsg_Resource_LoadingCompleted) {
                        ptr->status_.resolve(MaaStatus_Success);
                    }
                    else if (msg_str == MaaMsg_Resource_LoadingFailed) {
                        ptr->status_.resolve(MaaStatus_Failed);
                    }
                });
            },
            this);
    }

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    ~Resource() { MaaResourceDestroy(res_); }

    std::shared_ptr<Action> post_path(const std::string& path)
    {
        return put_action(MaaResourcePostPath(res_, path.c_str()));
    }

private:
    std::shared_ptr<Action> put_action(MaaResId id)
    {
        auto action = std::make_shared<Action>(shared_from_this(), id);
        actions_[id] = action->weak_from_this();
        return action;
    }

    MaaResourceHandle res_;
    std::map<MaaResId, std::weak_ptr<Action>> actions_;
};

}
