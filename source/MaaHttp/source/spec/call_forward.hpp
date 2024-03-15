#pragma once

#include "./utils.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::call
{

__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaSetImageEncoded::_1_data, std::string)

__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaResourceGetTaskList::_1_buffer, MaaStringBufferHandle)
__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaResourceGetHash::_1_buffer, MaaStringBufferHandle)
__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaControllerGetUUID::_1_buffer, MaaStringBufferHandle)

}; // namespace lhg::call
