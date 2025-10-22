#include "loader.h"

#include <fstream>
#include <iostream>

#include "../foundation/spec.h"

static maajs::ObjectType init(maajs::EnvType env)
{
    maajs::init(env);

    auto maa = maajs::ObjectType::New(env);

    maajs::BindValue(maa, "Global", load_global(env));
    maajs::BindValue(maa, "Job", load_job(env));
    maajs::BindValue(maa, "Resource", load_resource(env));
    maajs::BindValue(maa, "ImageJob", load_image_job(env));
    maajs::BindValue(maa, "Controller", load_controller(env));
    maajs::BindValue(maa, "AdbController", load_adb_controller(env));
    maajs::BindValue(maa, "TaskJob", load_task_job(env));
    maajs::BindValue(maa, "Tasker", load_tasker(env));
    maajs::BindValue(maa, "Context", load_context(env));

    auto constants = load_constant(env);
    for (const auto& [key, obj] : constants) {
        maajs::BindValue(maa, key.c_str(), obj);
    }

    return maa;
}

#ifdef MAA_JS_IMPL_IS_NODEJS
Napi::Object Init(Napi::Env env, Napi::Object)
{
    return ::init(env);
}

NODE_API_MODULE(maa, Init);
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
void init_module_maa(JSContext* ctx)
{
    maajs::EnvType env { ctx };

    auto maa = ::init(env);

    maajs::BindValue(env.Global(), "maa", maa);
}

void maa_rt_print(std::string str)
{
    std::cout << str << std::endl;
}

void maa_rt_save(std::string path, maajs::ArrayBufferType data)
{
    std::ofstream fout(path, std::ios_base::out | std::ios_base::binary);
    fout.write(static_cast<char*>(data.Data()), data.ByteLength());
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
    MAA_BIND_FUNC(globalObject, "save", maa_rt_save);
    MAA_BIND_FUNC(globalObject, "exit", maa_rt_exit);
}
#endif
