#pragma once

// #define MAA_JS_IMPL_IS_NODEJS
// #define MAA_JS_IMPL_IS_QUICKJS

#if !defined(MAA_JS_IMPL_IS_NODEJS) && !defined(MAA_JS_IMPL_IS_QUICKJS)
#define MAA_JS_IMPL_IS_NODEJS
// #define MAA_JS_IMPL_IS_QUICKJS
#endif

#include "spec/class.h"   // IWYU pragma: export
#include "spec/instric.h" // IWYU pragma: export
