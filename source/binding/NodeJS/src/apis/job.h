#pragma once

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
    maajs::ValueType get(maajs::EnvType env);

    constexpr static char name[] = "Job";

    static JobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::EnvType env, maajs::ObjectType proto);
    void gc_mark(maajs::NativeMarkerFunc marker) override;
};
