#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/convert.h"
#include "classes.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ResourceNative)

struct ResourceImpl
{
    MaaResource* resource;

    constexpr static bool hasCtor = true;

    static ResourceImpl* ctor([[maybe_unused]] const maajs::CallbackInfo& info)
    {
        auto handle = MaaResourceCreate(nullptr, nullptr);
        if (!handle) {
            return nullptr;
        }
        return new ResourceImpl { handle };
    }

    static void init_proto([[maybe_unused]] maajs::EnvType env, [[maybe_unused]] maajs::ConstObjectType proto);
};

maajs::ValueType status(const maajs::CallbackInfo& info)
{
    auto res = maajs::ResourceNative::take(info.This());

    MaaId id = maajs::JSConvert<uint64_t>::from_value(info.Env(), info[0]);
    auto status = MaaResourceStatus(res->resource, id);
    return maajs::MakeNumber(info.Env(), status);
}

void ResourceImpl::init_proto(maajs::EnvType env, [[maybe_unused]] maajs::ConstObjectType proto)
{
    maajs::BindValue(env, proto, "status", maajs::MakeFunction(env, "Resource.[status]", 1, status));
}

maajs::ValueType load_resource(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::ResourceNative::init(env, ctor);
    return ctor;
}
