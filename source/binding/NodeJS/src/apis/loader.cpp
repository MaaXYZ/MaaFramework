#include "loader.h"

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
int Init(JSContext* ctx, JSModuleDef* m)
{
    maajs::EnvType env = {
        JS_GetRuntime(ctx),
        ctx,
    };

    maajs::init(env);

    JS_SetModuleExport(ctx, m, "Global", load_global(env));
    JS_SetModuleExport(ctx, m, "Job", load_job(env));
    JS_SetModuleExport(ctx, m, "Resource", load_resource(env));

    return 0;
}

extern "C" JSModuleDef* js_init_module_maa(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, Init);
    if (!m) {
        return nullptr;
    }
    JS_AddModuleExport(ctx, m, "Global");
    JS_AddModuleExport(ctx, m, "Job");
    JS_AddModuleExport(ctx, m, "Resource");
    return m;
}
#endif
