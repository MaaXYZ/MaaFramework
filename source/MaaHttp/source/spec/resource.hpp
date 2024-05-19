#pragma once

#include "helper.hpp"
#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaResourceCreate

#define CURF maa::func_type_MaaResourceCreate

LHGHandle(CURF::ret, alloc);

LHGCallback(maa::callback_MaaAPICallback, CURF::_0_callback, CURF::_1_callback_arg, true);

#undef CURF

// MaaResourceDestroy

#define CURF maa::func_type_MaaResourceDestroy

LHGHandle(CURF::_0_res, free);

#undef CURF

// MaaResourceGetTaskList

#define CURF maa::func_type_MaaResourceGetTaskList

LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);
MaaDeclStringBuffer(_1_buffer, true);

#undef CURF

// MaaResourceGetHash

#define CURF maa::func_type_MaaResourceGetHash

LHGOuterState(CURF::_1_buffer, MaaStringBufferHandle);
MaaDeclStringBuffer(_1_buffer, true);

#undef CURF

}
