#include "loader.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

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
    maajs::BindValue(maa, "Win32Controller", load_win32_controller(env));
#ifdef __APPLE__
    maajs::BindValue(maa, "PlayCoverController", load_playcover_controller(env));
#endif
    maajs::BindValue(maa, "DbgController", load_dbg_controller(env));
    maajs::BindValue(maa, "CustomController", load_custom_controller(env));
    maajs::BindValue(maa, "TaskJob", load_task_job(env));
    maajs::BindValue(maa, "Tasker", load_tasker(env));
    maajs::BindValue(maa, "Context", load_context(env));

    auto constants = load_constant(env);
    for (const auto& [key, obj] : constants) {
        maajs::BindValue(maa, key.c_str(), obj);
    }

#ifdef MAA_JS_BUILD_CLIENT
    maajs::BindValue(maa, "Client", load_client(env));
#endif

#ifdef MAA_JS_BUILD_SERVER
    maajs::BindValue(maa, "Server", load_server(env));
#endif

#ifdef MAA_JS_BUILD_PLUGIN
    maajs::BindValue(maa, "Plugin", load_plugin(env));
#endif

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

void maa_rt_print(maajs::ValueType value)
{
    std::cout << value.ToString().Utf8Value() << std::endl;
}

maajs::ArrayBufferType maa_rt_read_file(maajs::EnvType env, std::string path)
{
    std::ifstream fin(path, std::ios_base::in | std::ios_base::binary);
    std::stringstream buf;
    buf << fin.rdbuf();
    std::string content = buf.str();

    auto result = maajs::ArrayBufferType::New(env, content.size());
    std::memcpy(result.Data(), content.c_str(), content.size());
    return result;
}

void maa_rt_write_file(std::string path, maajs::ArrayBufferType data)
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
    MAA_BIND_FUNC(globalObject, "readFile", maa_rt_read_file);
    MAA_BIND_FUNC(globalObject, "writeFile", maa_rt_write_file);
    MAA_BIND_FUNC(globalObject, "exit", maa_rt_exit);
}
#endif
