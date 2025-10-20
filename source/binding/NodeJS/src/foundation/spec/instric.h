#pragma once

#include "instric.forward.h" // IWYU pragma: export

#ifdef MAA_JS_IMPL_IS_NODEJS
#include "nodejs/instric.impl.h" // IWYU pragma: export
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
#include "quickjs/instric.impl.h" // IWYU pragma: export
#endif
