#pragma once

#include "../foundation/spec.h"

struct ExtContext : public maajs::NativeClassBase
{
    maajs::FunctionRefType jobCtor;
    maajs::FunctionRefType resourceCtor;

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
        auto global = env.Global();
        auto ctx = global["__MAA_JS_EXT_CTX"].AsValue();
        if (ctx.IsUndefined()) {
            auto ptr = new ExtContext;
            global["__MAA_JS_EXT_CTX"] = { env, maajs::NativePointerHolder::make(env, ptr) };
            return ptr;
        }
        else {
            return maajs::NativePointerHolder::take<ExtContext>(ctx.peek());
        }
        /*
        auto ptr = static_cast<ExtContext*>(JS_GetContextOpaque(env));
        if (!ptr) {
            ptr = new ExtContext {};
            JS_SetContextOpaque(env, ptr);
            JS_AddRuntimeFinalizer(env.runtime, +[](JSRuntime*, void* ptr) { delete static_cast<ExtContext*>(ptr); }, ptr);
        }
        return ptr;
        */
#endif
    }
};
