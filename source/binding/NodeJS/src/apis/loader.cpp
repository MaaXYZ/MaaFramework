#include "loader.h"

#include <iostream>

#include "../foundation/bridge.h"
#include "../foundation/macros.h"

#ifdef MAA_JS_IMPL_IS_NODEJS
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    maajs::init(env);

    exports["Global"] = load_global(env);
    exports["Job"] = load_job(env);
    exports["Resource"] = load_resource(env);

    return exports;
}

NODE_API_MODULE(maa, Init);
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
void init_module_maa(JSContext* ctx)
{
    maajs::EnvType env = {
        JS_GetRuntime(ctx),
        ctx,
    };

    maajs::init(env);

    auto maa = JS_NewObject(ctx);
    auto globalObject = JS_GetGlobalObject(ctx);

    maajs::BindValue(env, maa, "Global", load_global(env));
    maajs::BindValue(env, maa, "Job", load_job(env));
    maajs::BindValue(env, maa, "Resource", load_resource(env));

    maajs::BindValue(env, globalObject, "maa", maa);

    JS_FreeValue(ctx, globalObject);
}

void maa_rt_print(std::string str)
{
    std::cout << str << std::endl;
}

void maa_rt_exit(maajs::EnvType env, std::string result)
{
    QuickJSRuntimeBridgeInterface::get(env)->call_exit(result);
}

void init_module_sys(JSContext* ctx)
{
    maajs::EnvType env = {
        JS_GetRuntime(ctx),
        ctx,
    };

    auto globalObject = JS_GetGlobalObject(ctx);

    MAA_BIND_FUNC(env, globalObject, "print", maa_rt_print);
    MAA_BIND_FUNC(env, globalObject, "exit", maa_rt_exit);

    JS_FreeValue(ctx, globalObject);
}
#endif
