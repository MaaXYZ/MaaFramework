#pragma once

#include <map>
#include <optional>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

template <typename Handle>
struct WeakHandleMap
{
    std::map<Handle*, maajs::WeakObjectRefType> objs;

    void gc_mark([[maybe_unused]] maajs::NativeMarkerFunc marker)
    {
#ifdef MAA_JS_IMPL_IS_QUICKJS
        for (const auto& [_, val] : objs) {
            marker(val);
        }
#endif
    }

    std::optional<maajs::ObjectType> find(Handle* handle)
    {
        if (auto it = objs.find(handle); it != objs.end()) {
            auto val = it->second.Value();
            if (val.IsNull()) {
                objs.erase(it);
                return std::nullopt;
            }
            else {
                return val;
            }
        }
        else {
            return std::nullopt;
        }
    }

    void add(Handle* handle, maajs::ObjectType object) { objs[handle] = maajs::WeakRefObject(object); }

    void del(Handle* handle) { objs.erase(handle); }
};

template <typename Handle>
struct StrongHandleMap
{
    std::map<Handle*, maajs::ObjectRefType> objs;

    void gc_mark([[maybe_unused]] maajs::NativeMarkerFunc marker)
    {
#ifdef MAA_JS_IMPL_IS_QUICKJS
        for (const auto& [_, val] : objs) {
            marker(val);
        }
#endif
    }

    std::optional<maajs::ObjectType> find(Handle* handle)
    {
        if (auto it = objs.find(handle); it != objs.end()) {
            return it->second.Value();
        }
        else {
            return std::nullopt;
        }
    }

    void add(Handle* handle, maajs::ObjectType object) { objs[handle] = maajs::PersistentObject(object); }

    void del(Handle* handle) { objs.erase(handle); }
};

#if defined(MAA_JS_IMPL_IS_QUICKJS) || defined(MAA_JS_BUILD_SERVER)
template <typename Handle>
using HandleMap = StrongHandleMap<Handle>;
#else
template <typename Handle>
using HandleMap = WeakHandleMap<Handle>;
#endif

struct ExtContext : public maajs::NativeClassBase
{
    maajs::FunctionRefType jobCtor;
    maajs::FunctionRefType resourceCtor;
    maajs::FunctionRefType imageJobCtor;
    maajs::FunctionRefType controllerCtor;
    maajs::FunctionRefType adbControllerCtor;
    maajs::FunctionRefType win32ControllerCtor;
    maajs::FunctionRefType playcoverControllerCtor;
    maajs::FunctionRefType dbgControllerCtor;
    maajs::FunctionRefType customControllerCtor;
    maajs::FunctionRefType taskJobCtor;
    maajs::FunctionRefType taskerCtor;
    maajs::FunctionRefType contextCtor;
    std::optional<maajs::FunctionRefType> clientCtor;

    HandleMap<MaaResource> resources;
    HandleMap<MaaController> controllers;
    HandleMap<MaaTasker> taskers;

    std::vector<std::unique_ptr<maajs::CallbackContext>> globalCallbacks;

    void gc_mark(maajs::NativeMarkerFunc marker) override
    {
        marker(jobCtor.Value());
        marker(resourceCtor.Value());
        marker(imageJobCtor.Value());
        marker(controllerCtor.Value());
        marker(adbControllerCtor.Value());
        marker(win32ControllerCtor.Value());
        marker(playcoverControllerCtor.Value());
        marker(dbgControllerCtor.Value());
        marker(customControllerCtor.Value());
        marker(taskJobCtor.Value());
        marker(taskerCtor.Value());
        marker(contextCtor.Value());
        if (clientCtor) {
            marker(clientCtor->Value());
        }

        resources.gc_mark(marker);
        controllers.gc_mark(marker);
        taskers.gc_mark(marker);

        for (const auto& ptr : globalCallbacks) {
            marker(ptr->fn);
        }
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
            return maajs::NativePointerHolder::take<ExtContext>(env, ctx.peek());
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
