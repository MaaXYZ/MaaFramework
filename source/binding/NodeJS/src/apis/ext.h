#pragma once

#include "../foundation/spec.h"

struct ExtContext : public maajs::NativeClassBase
{
    maajs::FunctionRefType jobCtor;
    maajs::FunctionRefType resourceCtor;

    ~ExtContext()
    {
        jobCtor.Unref();
        resourceCtor.Unref();
    }

    void gc_mark([[maybe_unused]] maajs::NativeMarkerFunc marker) override
    {
        marker(jobCtor.Value());
        marker(resourceCtor.Value());
    }

    static ExtContext* get(maajs::EnvType env)
    {
#ifdef MAA_JS_IMPL_IS_NODEJS
        auto ptr = env.GetInstanceData<ExtContext>();
        if (!ptr) {
            ptr = new ExtContext {};
            env.SetInstanceData(ptr);
        }
        return ptr;
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
        auto global = JS_GetGlobalObject(env.context);
        auto ctx = JS_GetPropertyStr(env.context, global, "__MAA_JS_EXT_CTX");

        if (JS_IsUndefined(ctx)) {
            JS_FreeValue(env.context, ctx);
            auto ptr = new ExtContext;
            ctx = maajs::NativePointerHolder::make(env.context, ptr);
            JS_SetPropertyStr(env.context, global, "__MAA_JS_EXT_CTX", ctx);
            JS_FreeValue(env.context, global);
            return ptr;
        }
        else {
            JS_FreeValue(env.context, global);
            auto ptr = maajs::NativePointerHolder::take<ExtContext>(ctx);
            JS_FreeValue(env.context, ctx);
            return ptr;
        }
        /*
        auto ptr = static_cast<ExtContext*>(JS_GetContextOpaque(env.context));
        if (!ptr) {
            ptr = new ExtContext {};
            JS_SetContextOpaque(env.context, ptr);
            JS_AddRuntimeFinalizer(env.runtime, +[](JSRuntime*, void* ptr) { delete static_cast<ExtContext*>(ptr); }, ptr);
        }
        return ptr;
        */
#endif
    }
};
