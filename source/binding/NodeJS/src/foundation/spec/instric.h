#pragma once

#include "instric.forward.h" // IWYU pragma: export

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/instric.impl.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/instric.impl.h" // IWYU pragma: export
#endif

namespace maajs
{

inline FunctionType MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::shared_ptr<ObjectRefType> capture)
{
    return MakeFunction(env, name, argc, func, [capture](auto marker) { marker(capture->Value()); });
}

inline FunctionType MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::shared_ptr<FunctionRefType> capture)
{
    return MakeFunction(env, name, argc, func, [capture](auto marker) { marker(capture->Value()); });
}

}
