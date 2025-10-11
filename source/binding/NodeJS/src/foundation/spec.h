#pragma once

// #define MAA_JS_IMPL_IS_NODEJS
// #define MAA_JS_IMPL_IS_QUICKJS

#if !defined(MAA_JS_IMPL_IS_NODEJS) && !defined(MAA_JS_IMPL_IS_QUICKJS)
#define MAA_JS_IMPL_IS_QUICKJS
#endif

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "spec.nodejs.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "spec.quickjs.h" // IWYU pragma: export
#endif
