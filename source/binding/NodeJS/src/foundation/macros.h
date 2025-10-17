#pragma once

#include "convert.h"

#define MAA_WRAP_FUNC(func) maajs::WrapFunctionHelper<decltype(&func), &func, #func>::make()
#define MAA_WRAP_FUNC_VALUE(env, func) maajs::WrapFunctionHelper<decltype(&func), &func, #func>::makeValue(env)

#define MAA_BIND_FUNC(object, prop, func) maajs::BindValue(object, prop, MAA_WRAP_FUNC_VALUE(env, func))
#define MAA_BIND_GETTER(object, prop, func) maajs::BindGetter(object, prop, #func, MAA_WRAP_FUNC(func))
