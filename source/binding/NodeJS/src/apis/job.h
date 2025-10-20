#pragma once

#include <optional>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

struct JobImpl : public maajs::NativeClassBase
{
    maajs::ObjectRefType source;
    MaaId id;
    std::optional<MaaStatus> status;

    maajs::ValueType get_source();
    MaaId get_id();
    MaaStatus get_status();
    maajs::PromiseType wait(maajs::ValueType self, maajs::EnvType env);
    virtual maajs::ValueType get();

    std::string to_string() override;

    constexpr static char name[] = "Job";

    static JobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::ObjectType proto, maajs::FunctionType);
    void gc_mark(maajs::NativeMarkerFunc marker) override;
};

MAA_JS_NATIVE_CLASS_STATIC_FORWARD(JobImpl)
