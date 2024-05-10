// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <MaaToolkit/MaaToolkitAPI.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/Resource.hpp"
#include "MaaPP/maa/SyncContext.hpp"
#include "MaaPP/maa/Type.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"
#include "MaaPP/maa/details/String.hpp"

#endif

namespace maa
{

MAAPP_EXPORT class CustomRecognizer : public std::enable_shared_from_this<CustomRecognizer>
{
    friend class Instance;

public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<CustomRecognizer>(std::forward<Args>(args)...);
    }

    using analyze_func = std::function<coro::Promise<AnalyzeResult>(
        std::shared_ptr<SyncContext>,
        std::shared_ptr<details::Image>,
        std::string_view,
        std::string_view)>;

    CustomRecognizer(analyze_func analyze)
        : analyze_(analyze)
    {
    }

private:
    static MaaBool run_analyze(
        MaaSyncContextHandle sync_context,
        const MaaImageBufferHandle image,
        MaaStringView task_name,
        MaaStringView custom_recognition_param,
        MaaTransparentArg recognizer_arg,
        /*out*/ MaaRectHandle out_box,
        /*out*/ MaaStringBufferHandle out_detail)
    {
        auto self = reinterpret_cast<CustomRecognizer*>(recognizer_arg)->shared_from_this();
        auto image_handle = details::Image::make(image);
        auto result = self->analyze_(
                              SyncContext::make(sync_context),
                              image_handle,
                              task_name,
                              custom_recognition_param)
                          .sync_wait();
        *out_box = result.rec_box;
        (details::String(out_detail)) = result.rec_detail;
        return result.result;
    }

    constexpr static MaaCustomRecognizerAPI api_ = { &CustomRecognizer::run_analyze };
    analyze_func analyze_;
};

MAAPP_EXPORT class CustomAction : public std::enable_shared_from_this<CustomAction>
{
    friend class Instance;

public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<CustomAction>(std::forward<Args>(args)...);
    }

    using run_func = std::function<coro::Promise<bool>(
        std::shared_ptr<SyncContext>,
        MaaStringView,
        MaaStringView,
        const MaaRect&,
        MaaStringView)>;

    CustomAction(run_func run)
        : run_(run)
    {
    }

private:
    static MaaBool run_run(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView custom_action_param,
        MaaRectHandle cur_box,
        MaaStringView cur_rec_detail,
        MaaTransparentArg action_arg)
    {
        auto self = reinterpret_cast<CustomAction*>(action_arg)->shared_from_this();
        return self
            ->run_(
                SyncContext::make(sync_context),
                task_name,
                custom_action_param,
                *cur_box,
                cur_rec_detail)
            .sync_wait();
    }

    constexpr static MaaCustomActionAPI api_ = { &CustomAction::run_run, nullptr };
    run_func run_;
};

MAAPP_EXPORT class Instance;

MAAPP_EXPORT class InstanceAction : public details::ActionBase<InstanceAction, Instance>
{
    friend class Instance;

public:
    using ActionBase::ActionBase;

    MaaStatus status();
    bool set_param(const json::object& param);

    coro::Promise<MaaStatus> wait() { return status_; }

private:
    coro::Promise<MaaStatus> status_;
};

MAAPP_EXPORT class Instance
    : public details::ActionHelper<Instance, InstanceAction, MaaInstanceHandle>
{
    friend class InstanceAction;

public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<Instance>(std::forward<Args>(args)...);
    }

    Instance(std::function<void(std::shared_ptr<message::MessageBase>)> callback = nullptr)
        : ActionHelper(MaaCreate(&Instance::_callback, this))
        , user_callback_(callback)
    {
    }

    ~Instance()
    {
        MaaDestroy(inst_);
        for (auto action : actions_ | std::views::values) {
            action.lock()->wait().sync_wait();
        }
    }

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

    bool bind(const std::string& name, std::shared_ptr<CustomRecognizer> reco)
    {
        custom_recognizers_[name] = reco;
        return MaaRegisterCustomRecognizer(
            inst_,
            name.c_str(),
            const_cast<MaaCustomRecognizerAPI*>(&CustomRecognizer::api_),
            reco.get());
    }

    bool bind(const std::string& name, std::shared_ptr<CustomAction> act)
    {
        custom_actions_[name] = act;
        return MaaRegisterCustomAction(
            inst_,
            name.c_str(),
            const_cast<MaaCustomActionAPI*>(&CustomAction::api_),
            act.get());
    }

    bool unbind_recognizer(const std::string& name)
    {
        custom_recognizers_.erase(name);
        return MaaUnregisterCustomRecognizer(inst_, name.c_str());
    }

    bool unbind_action(const std::string& name)
    {
        custom_actions_.erase(name);
        return MaaUnregisterCustomAction(inst_, name.c_str());
    }

    bool unbind_recognizer()
    {
        custom_recognizers_.clear();
        return MaaClearCustomRecognizer(inst_);
    }

    bool unbind_action()
    {
        custom_actions_.clear();
        return MaaClearCustomAction(inst_);
    }

    bool bind_recognizer_executor(
        const std::string& name,
        const std::string& path,
        const std::vector<std::string>& argv)
    {
        std::vector<MaaStringView> argv_view;
        std::transform(
            argv.begin(),
            argv.end(),
            std::back_insert_iterator<std::vector<MaaStringView>>(argv_view),
            [](const std::string& str) { return str.c_str(); });
        return MaaToolkitRegisterCustomRecognizerExecutor(
            inst_,
            name.c_str(),
            path.c_str(),
            argv_view.data(),
            argv_view.size());
    }

    bool bind_action_executor(
        const std::string& name,
        const std::string& path,
        const std::vector<std::string>& argv)
    {
        std::vector<MaaStringView> argv_view;
        std::transform(
            argv.begin(),
            argv.end(),
            std::back_insert_iterator<std::vector<MaaStringView>>(argv_view),
            [](const std::string& str) { return str.c_str(); });
        return MaaToolkitRegisterCustomActionExecutor(
            inst_,
            name.c_str(),
            path.c_str(),
            argv_view.data(),
            argv_view.size());
    }

    bool unbind_recognizer_executor(const std::string& name)
    {
        return MaaToolkitUnregisterCustomRecognizerExecutor(inst_, name.c_str());
    }

    bool unbind_action_executor(const std::string& name)
    {
        return MaaToolkitUnregisterCustomActionExecutor(inst_, name.c_str());
    }

    bool unbind_recognizer_executor() { return MaaToolkitClearCustomRecognizerExecutor(inst_); }

    bool unbind_action_executor() { return MaaToolkitClearCustomActionExecutor(inst_); }

    bool inited() { return MaaInited(inst_); }

    bool running() { return MaaRunning(inst_); }

    bool stop() { return MaaPostStop(inst_); }

    auto controller() { return controller_; }

    auto resource() { return resource_; }

private:
    static void _callback(MaaStringView msg, MaaStringView details, MaaTransparentArg arg)
    {
        auto msg_ptr = message::parse(msg, details);

        auto self = reinterpret_cast<Instance*>(arg)->shared_from_this();

        coro::EventLoop::current()->defer([self, msg_ptr]() {
            if (auto msg = msg_ptr->is<message::InstanceTaskMessage>()) {
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
                if (msg->type == message::InstanceTaskMessage::Completed) {
                    ptr->status_.resolve(MaaStatus_Success);
                }
                else if (msg->type == message::InstanceTaskMessage::Failed) {
                    ptr->status_.resolve(MaaStatus_Failed);
                }
            }
        });

        if (self->user_callback_) {
            coro::EventLoop::current()->defer([self, msg_ptr]() { self->user_callback_(msg_ptr); });
        }
    }

    std::function<void(std::shared_ptr<message::MessageBase>)> user_callback_;
    std::shared_ptr<Controller> controller_;
    std::shared_ptr<Resource> resource_;
    std::map<std::string, std::shared_ptr<CustomRecognizer>> custom_recognizers_;
    std::map<std::string, std::shared_ptr<CustomAction>> custom_actions_;
};

inline MaaStatus InstanceAction::status()
{
    return MaaTaskStatus(inst_->inst_, id_);
}

inline bool InstanceAction::set_param(const json::object& param)
{
    return MaaSetTaskParam(inst_->inst_, id_, param.to_string().c_str());
}

}
