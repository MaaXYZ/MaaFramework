#pragma once

#include "./utils.hpp"

#include "MaaFramework/MaaDef.h"
#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::callback
{

__CALLBACK_DECLARE_OUTER_STATE(
    maa::func_type_CustomActionRun::_0_sync_context,
    HandleManager<MaaSyncContextHandle>::ScopedHandle)

__CALLBACK_DECLARE_OUTER_STATE(
    maa::func_type_CustomRecognizerAnalyze::_0_sync_context,
    HandleManager<MaaSyncContextHandle>::ScopedHandle)

__CALLBACK_DECLARE_OUTER_STATE(
    maa::func_type_CustomRecognizerAnalyze::_1_image,
    HandleManager<MaaImageBufferHandle>::ScopedHandle)

}; // namespace lhg::callback
