#pragma once

#include "job.h"

struct ImageJobImpl : public JobImpl
{
    virtual maajs::ValueType get(maajs::EnvType env) override;

    constexpr static char name[] = "ImageJob";

    static ImageJobImpl* ctor(const maajs::CallbackInfo& info);
    static void init_proto(maajs::EnvType env, maajs::ObjectType proto);
};
