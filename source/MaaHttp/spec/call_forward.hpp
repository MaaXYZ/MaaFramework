#pragma once

#include "./utils.hpp"

#include "utils/phony.hpp"

#include "../info.hpp"

namespace lhg::call
{

__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaSetImageEncoded::_1_data, std::string)

#define __DECLARE_STRING_BUFFER_OUTPUT(func_tag, arg_tag) \
    __CALL_DECLARE_OUTER_STATE(func_tag::arg_tag, MaaStringBufferHandle)

__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetTaskList, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaResourceGetHash, _1_buffer)
__DECLARE_STRING_BUFFER_OUTPUT(maa::func_type_MaaControllerGetUUID, _1_buffer)

#undef __DECLARE_STRING_BUFFER_OUTPUT

}; // namespace lhg::call
