#pragma once

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/callback.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/callback.h" // IWYU pragma: export
#endif
