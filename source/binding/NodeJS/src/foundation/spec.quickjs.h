#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <quickjs.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "traits.h"

namespace maa_js
{

using ValueType = JSValue;

struct ContextType
{
    JSRuntime* runtime;
    JSContext* context;
};

inline ValueType get_undefined(ContextType)
{
    return JS_UNDEFINED;
}

template <typename Type, void (*Finalizer)(ContextType, Type*)>
struct ExternalType
{
    static JSClassID _classId;

    JSObject* _impl = nullptr;

    static void init(ContextType ctx)
    {
        static JSClassDef classDef = {
            .name = ExternalTraits<Type>::name,
            .finalizer = +[](JSRuntime* runtime, Type* data) { Finalizer({ runtime, nullptr }, data); },
        };

        JS_NewClassID(ctx, &_classId);
        JS_NewClass(ctx, &_classId, &classDef);
    }

    static ExternalType create(ContextType env, Type* data)
    {
        auto obj = JS_NewObjectClass(env.context, _classId);
        JS_SetOpaque(obj, data);
        return { obj };
    };

    static ExternalType cast(ValueType value) { return { value }; }

    static bool check(ValueType value) { return JS_IsObject(value) && JS_GetClassID(value) == _classId; }

    Type* get() const { return JS_GetOpaque(_impl, _classId); }
};

}
