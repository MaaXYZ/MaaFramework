#pragma once

#include <functional>

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/types.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/types.h" // IWYU pragma: export
#endif

namespace maajs
{

using RawCallback = std::function<ValueType(const CallbackInfo&)>;

using NativeMarkerFunc = std::function<void(const ValueType& value)>;

struct NativeClassBase
{
    EnvType env { nullptr };

    NativeClassBase() = default;
    virtual ~NativeClassBase() = default;

    virtual void init_bind([[maybe_unused]] ObjectType self) {}

    virtual void gc_mark([[maybe_unused]] NativeMarkerFunc marker) {}

    virtual std::string to_string() { return ""; }
};

}
