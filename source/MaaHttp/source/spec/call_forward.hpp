#pragma once

#include "utils/phony.hpp"

#include "../info.hpp"

LHGArgOuterState(maa::func_type_MaaSetImageEncoded::_1_data, std::string);

LHGArgOuterState(maa::func_type_MaaResourceGetTaskList::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaResourceGetHash::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaControllerGetUUID::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(
    maa::func_type_MaaSyncContextGetTaskResult::_2_out_task_result,
    MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognizer::_5_out_detail, MaaStringBufferHandle);

LHGArgOuterState(maa::func_type_MaaSyncContextRunAction::_3_cur_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognizer::_4_out_box, MaaRect);

LHGArgOuterState(maa::func_type_MaaGetRecognitionResult::_2_hit, MaaBool);
LHGArgOuterState(maa::func_type_MaaGetRecognitionResult::_3_hit_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaGetRecognitionResult::_4_hit_detail, MaaStringBufferHandle);
