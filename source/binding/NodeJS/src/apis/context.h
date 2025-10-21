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

    void override_pipeline(maajs::ValueType pipeline);
    void override_next(std::string node_name, std::vector<std::string> next_list);
    std::optional<std::string> get_node_data(std::string node_name);
    std::optional<maajs::ValueType> get_node_data_parsed(std::string node_name);
    MaaTaskId get_task_id();
    maajs::ValueType get_tasker();
    maajs::ValueType clone();

    std::string to_string() override;

    constexpr static char name[] = "Context";

    static ContextImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType ctor);
};

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(ContextImpl)
