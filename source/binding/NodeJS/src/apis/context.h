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
    maajs::PromiseType run_recognition_direct(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string reco_type,
        maajs::ValueType reco_param,
        maajs::ArrayBufferType image);
    maajs::PromiseType run_action_direct(
        maajs::ValueType self,
        maajs::EnvType env,
        std::string action_type,
        maajs::ValueType action_param,
        MaaRect box,
        std::string reco_detail);
    void override_pipeline(maajs::ValueType pipeline);
    void override_next(std::string node_name, std::vector<std::string> next_list);
    void override_image(std::string image_name, maajs::ArrayBufferType image);
    std::optional<std::string> get_node_data(std::string node_name);
    std::optional<maajs::ValueType> get_node_data_parsed(std::string node_name);
    MaaTaskId get_task_id();
    maajs::ValueType get_tasker();
    maajs::ValueType clone();
    void set_anchor(std::string anchor_name, std::string node_name);
    std::optional<std::string> get_anchor(std::string anchor_name);
    int32_t get_hit_count(std::string node_name);
    void clear_hit_count(std::string node_name);
    bool wait_freezes(
        maajs::OptionalParam<int64_t> time,
        maajs::OptionalParam<std::optional<MaaRect>> roi,
        maajs::OptionalParam<maajs::ValueType> other_param);

    std::string to_string() override;

    // 写作locate, 实际上总是create
    static maajs::ValueType locate_object(maajs::EnvType env, MaaContext* ctx);

    constexpr static char name[] = "Context";

    static ContextImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

