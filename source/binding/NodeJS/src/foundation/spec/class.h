#pragma once

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/class.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/class.h" // IWYU pragma: export
#endif
