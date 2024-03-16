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
__CALL_DECLARE_OUTER_STATE(
    maa::func_type_MaaSyncContextGetTaskResult::_2_out_task_result,
    MaaStringBufferHandle)
__CALL_DECLARE_OUTER_STATE(
    maa::func_type_MaaSyncContextRunRecognizer::_5_out_detail,
    MaaStringBufferHandle)

__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaSyncContextRunAction::_3_cur_box, MaaRect);
__CALL_DECLARE_OUTER_STATE(maa::func_type_MaaSyncContextRunRecognizer::_4_out_box, MaaRect);

}; // namespace lhg::call
