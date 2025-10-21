#pragma once

// #define MAA_JS_IMPL_IS_NODEJS
// #define MAA_JS_IMPL_IS_QUICKJS

#if !defined(MAA_JS_IMPL_IS_NODEJS) && !defined(MAA_JS_IMPL_IS_QUICKJS)
#define MAA_JS_IMPL_IS_NODEJS
// #define MAA_JS_IMPL_IS_QUICKJS
#endif

#include "spec/async.h"    // IWYU pragma: export
#include "spec/callback.h" // IWYU pragma: export
#include "spec/class.h"    // IWYU pragma: export
#include "spec/convert.h"  // IWYU pragma: export
#include "spec/instric.h"  // IWYU pragma: export
#include "spec/utils.h"    // IWYU pragma: export
#include "spec/wrapper.h"  // IWYU pragma: export

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "spec/quickjs/bridge.h" // IWYU pragma: export
#endif

#include "jsutils.h" // IWYU pragma: export
