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
    exports["ImageJob"] = load_image_job(env);
    exports["Controller"] = load_controller(env);
    exports["AdbController"] = load_adb_controller(env);

    return exports;
}

NODE_API_MODULE(maa, Init);
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
void init_module_maa(JSContext* ctx)
{
    maajs::EnvType env { ctx };
    maajs::init(env);

    auto maa = maajs::ObjectType::New(env);
    auto globalObject = env.Global();

    maajs::BindValue(maa, "Global", load_global(env));
    maajs::BindValue(maa, "Job", load_job(env));
    maajs::BindValue(maa, "Resource", load_resource(env));
    maajs::BindValue(maa, "ImageJob", load_image_job(env));
    maajs::BindValue(maa, "Controller", load_controller(env));
    maajs::BindValue(maa, "AdbController", load_adb_controller(env));

    maajs::BindValue(globalObject, "maa", maa);
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
    maajs::EnvType env { ctx };
    auto globalObject = env.Global();

    MAA_BIND_FUNC(globalObject, "print", maa_rt_print);
    MAA_BIND_FUNC(globalObject, "exit", maa_rt_exit);
}
#endif
