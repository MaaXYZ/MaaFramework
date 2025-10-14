#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

maajs::ValueType version_from_macro(const maajs::CallbackInfo& info)
{
    return maajs::MakeString(info.Env(), MAA_VERSION);
}

maajs::ValueType version(const maajs::CallbackInfo& info)
{
    return maajs::MakeString(info.Env(), MaaVersion());
}

maajs::ObjectType load_global(maajs::EnvType env)
{
    auto globalObject = maajs::MakeObject(env);

    maajs::BindGetter(env, globalObject, "version_from_macro", "Global.[version_from_macro getter]", version_from_macro);
    maajs::BindGetter(env, globalObject, "version", "Global.[version getter]", version);

    return globalObject;
}
