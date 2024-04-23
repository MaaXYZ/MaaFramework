#pragma once

#include <iostream>
#include <memory>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Controller.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/Resource.hpp"
#include "MaaPP/maa/SyncContext.hpp"
#include "MaaPP/maa/details/ActionHelper.hpp"
#include "MaaPP/maa/details/String.hpp"

namespace maa
{

class CustomRecognizer : public std::enable_shared_from_this<CustomRecognizer>
{
    friend class Instance;

public:
    struct AnalyzeResult
    {
        bool result;
        MaaRect rec_box;
        std::string rec_detail;
    };

    using analyze_func = std::function<coro::Promise<
        AnalyzeResult>(std::shared_ptr<SyncContext>, details::Image, MaaStringView, MaaStringView)>;

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
        auto result = self->analyze_(
                              SyncContext::make(sync_context),
                              image,
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

class CustomAction : public std::enable_shared_from_this<CustomAction>
{
    friend class Instance;

public:
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

class Instance;

class InstanceAction : public details::ActionBase<InstanceAction, Instance>
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

class Instance : public details::ActionHelper<Instance, InstanceAction, MaaInstanceHandle>
{
    friend class InstanceAction;

public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<Instance>(std::forward<Args>(args)...);
    }

    Instance(
        std::function<void(std::string_view msg, const json::object& details)> callback = nullptr)
        : ActionHelper(MaaCreate(&Instance::_callback, this))
        , user_callback_(callback)
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

    bool inited() { return MaaInited(inst_); }

    bool running() { return MaaRunning(inst_); }

    bool stop() { return MaaPostStop(inst_); }

    auto controller() { return controller_; }

    auto resource() { return resource_; }

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
                if (!detail_val.is_object()) {
                    return;
                }
                const auto& detail_obj = detail_val.as_object();
                if (self->user_callback_) {
                    self->user_callback_(msg_str, detail_obj);
                }
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

    std::function<void(std::string_view msg, const json::object& details)> user_callback_;
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
