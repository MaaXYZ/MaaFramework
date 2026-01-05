#pragma once

#include <map>
#include <optional>
#include <string>

#include <MaaFramework/MaaAPI.h>

#include "controller.h"
#include "resource.h"

struct TaskJobImpl : public JobImpl
{
    virtual maajs::ValueType get() override;

    constexpr static char name[] = "TaskJob";

    static TaskJobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

struct TaskerImpl : public maajs::NativeClassBase
{
    MaaTasker* tasker {};
    bool own = false;
    std::map<MaaSinkId, maajs::CallbackContext*> sinks {};
    std::map<MaaSinkId, maajs::CallbackContext*> ctxSinks {};

    TaskerImpl() = default;
    TaskerImpl(MaaTasker* res, bool own);
    ~TaskerImpl();
    void destroy();
    MaaSinkId add_sink(maajs::FunctionType sink);
    void remove_sink(MaaSinkId id);
    void clear_sinks();
    MaaSinkId add_context_sink(maajs::FunctionType sink);
    void remove_context_sink(MaaSinkId id);
    void clear_context_sinks();
    maajs::ValueType post_task(maajs::ValueType self, maajs::EnvType env, std::string entry, maajs::OptionalParam<maajs::ValueType> param);
    maajs::ValueType post_recognition(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string reco_type,
        maajs::ValueType reco_param,
        maajs::ArrayBufferType image);
    maajs::ValueType post_action(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string action_type,
        maajs::ValueType action_param,
        MaaRect box,
        maajs::OptionalParam<std::string> reco_detail);
    maajs::ValueType post_stop(maajs::ValueType self, maajs::EnvType env);
    MaaStatus status(MaaResId id);
    maajs::PromiseType wait(MaaResId id);
    bool get_inited();
    bool get_running();
    bool get_stopping();
    void set_resource(std::optional<maajs::NativeObject<ResourceImpl>> res);
    std::optional<maajs::ValueType> get_resource();
    void set_controller(std::optional<maajs::NativeObject<ControllerImpl>> ctrl);
    std::optional<maajs::ValueType> get_controller();
    void clear_cache();
    std::optional<maajs::ValueType> recognition_detail(MaaRecoId id);
    std::optional<maajs::ValueType> action_detail(MaaActId id);
    std::optional<maajs::ValueType> node_detail(MaaNodeId id);
    std::optional<maajs::ValueType> task_detail(MaaTaskId id);
    std::optional<MaaNodeId> latest_node(std::string node_name);

    std::string to_string() override;

    static maajs::ValueType locate_object(maajs::EnvType env, MaaTasker* tsk);

    constexpr static char name[] = "Tasker";

    virtual void init_bind(maajs::ObjectType self) override;
    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static TaskerImpl* ctor(const maajs::CallbackInfo&);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType);
};

