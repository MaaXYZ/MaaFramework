#pragma once

#include "utils/phony.hpp"

#include "../info.hpp"

LHGArgOuterState(maa::func_type_MaaSetImageEncoded::_1_data, std::string);

LHGArgOuterState(maa::func_type_MaaResourceGetTaskList::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaResourceGetHash::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaControllerGetUUID::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognizer::_5_out_detail, MaaStringBufferHandle);

LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_1_hit, MaaBool);
LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_2_hit_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_3_detail_json, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaQueryRunningDetail::_1_reco_id, MaaRecoId);
LHGArgOuterState(maa::func_type_MaaQueryRunningDetail::_2_successful, MaaBool)

    LHGArgOuterState(maa::func_type_MaaSyncContextRunAction::_3_cur_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognizer::_4_out_box, MaaRect);
