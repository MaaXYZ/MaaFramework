// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"

#endif

namespace maa::details
{

MAAPP_EXPORT template <typename IAction, typename IActionHelper>
struct ActionBase : public std::enable_shared_from_this<IAction>
{
    ActionBase(std::shared_ptr<IActionHelper> inst, MaaId id)
        : inst_(inst)
        , id_(id)
    {
    }

    ~ActionBase() { inst_->actions_.erase(id_); }

    ActionBase(const ActionBase&) = delete;
    ActionBase& operator=(const ActionBase&) = delete;

    std::shared_ptr<IActionHelper> inst_;
    MaaId id_;
};

MAAPP_EXPORT template <typename IActionHelper, typename IAction, typename Handle>
class ActionHelper : public std::enable_shared_from_this<IActionHelper>
{
    friend struct ActionBase<IAction, IActionHelper>;

protected:
    ActionHelper(Handle inst)
        : inst_(inst)
    {
    }

    ~ActionHelper() {}

    std::shared_ptr<IAction> put_action(MaaId id)
    {
        if (id == MaaInvalidId) {
            return nullptr;
        }
        auto action = std::make_shared<IAction>(this->shared_from_this(), id);
        actions_[id] = action->weak_from_this();
        return action;
    }

    template <typename... Args>
    std::shared_ptr<IAction> put_action(MaaId id, Args&&... args)
    {
        auto action =
            std::make_shared<IAction>(this->shared_from_this(), id, std::forward<Args>(args)...);
        actions_[id] = action->weak_from_this();
        return action;
    }

public:
    ActionHelper(const ActionHelper&) = delete;
    ActionHelper& operator=(const ActionHelper&) = delete;

protected:
    Handle inst_;
    std::map<MaaId, std::weak_ptr<IAction>> actions_;
};

}
