#pragma once

#include "helper.hpp"
#include "type.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::lifecycle
{

// MaaSyncContextRunRecognition

#define CURF maa::func_type_MaaSyncContextRunRecognition

LHGOuterState(CURF::_4_out_box, MaaRect);
LHGOuterState(CURF::_5_out_detail, MaaStringBufferHandle);

LHGOutput(CURF::_4_out_box);

MaaDeclStringBuffer(_5_out_detail, true);
MaaDeclRectOut(_4_out_box, true);

#undef CURF

// MaaSyncContextRunAction

#define CURF maa::func_type_MaaSyncContextRunAction

LHGOuterState(CURF::_3_cur_box, MaaRect);

MaaDeclRectIn(_3_cur_box, true);

#undef CURF

}
