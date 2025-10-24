#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

struct ContextImpl : public maajs::NativeClassBase
{
    MaaContext* context {};
    std::vector<std::unique_ptr<maajs::ObjectRefType>> cloned_contexts;

    ContextImpl() = default;
    ContextImpl(MaaContext* ctx);

    maajs::PromiseType
        run_task(maajs::ValueType self, maajs::EnvType env, std::string entry, maajs::OptionalParam<maajs::ValueType> pipeline_override);
    maajs::PromiseType run_recognition(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string entry,
        maajs::ArrayBufferType image,
        maajs::OptionalParam<maajs::ValueType> pipeline_override);
    maajs::PromiseType run_action(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string entry,
        MaaRect box,
        std::string reco_detail,
        maajs::OptionalParam<maajs::ValueType> pipeline_override);
    void override_pipeline(maajs::ValueType pipeline);
    void override_next(std::string node_name, std::vector<std::string> next_list);
    std::optional<std::string> get_node_data(std::string node_name);
    std::optional<maajs::ValueType> get_node_data_parsed(std::string node_name);
    MaaTaskId get_task_id();
    maajs::ValueType get_tasker();
    maajs::ValueType clone();

    void recursive_clean();

    std::string to_string() override;

    // 写作locate, 实际上总是create
    static maajs::ValueType locate_object(maajs::EnvType env, MaaContext* ctx);

    constexpr static char name[] = "Context";

    virtual void gc_mark(maajs::NativeMarkerFunc marker) override;
    static ContextImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

struct ScopedContextHolder
{
    maajs::ValueType value;

    ~ScopedContextHolder();
};
