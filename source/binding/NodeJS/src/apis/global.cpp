#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/macros.h"
#include "../foundation/spec.h"

std::string version_from_macro()
{
    return MAA_VERSION;
}

std::string version()
{
    return MaaVersion();
}

maajs::ObjectType load_global(maajs::EnvType env)
{
    auto globalObject = maajs::ObjectType::New(env);

    MAA_BIND_GETTER(env, globalObject, "version_from_macro", version_from_macro);
    MAA_BIND_GETTER(env, globalObject, "version", version);

    return globalObject;
}
