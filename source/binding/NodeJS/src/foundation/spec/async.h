#pragma once

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/async.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/async.h" // IWYU pragma: export
#endif
