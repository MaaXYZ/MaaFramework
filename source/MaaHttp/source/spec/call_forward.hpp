#pragma once

#include <string>
#include <tuple>
#include <vector>

#include "utils/phony.hpp"

#include "../info.hpp"

LHGArgOuterState(maa::func_type_MaaSetImageEncoded::_1_data, std::string);

LHGArgOuterState(maa::func_type_MaaResourceGetTaskList::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaResourceGetHash::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaControllerGetUUID::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognition::_5_out_detail, MaaStringBufferHandle);

LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_1_hit, MaaBool);
LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_2_hit_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaQueryRecognitionDetail::_3_detail_json, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaQueryNodeDetail::_1_reco_id, MaaRecoId);
LHGArgOuterState(maa::func_type_MaaQueryNodeDetail::_2_run_completed, MaaBool);

LHGArgOuterState(maa::func_type_MaaSyncContextRunAction::_3_cur_box, MaaRect);
LHGArgOuterState(maa::func_type_MaaSyncContextRunRecognition::_4_out_box, MaaRect);

LHGArgOuterState(maa::func_type_MaaToolkitGetWindowClassName::_1_buffer, MaaStringBufferHandle);
LHGArgOuterState(maa::func_type_MaaToolkitGetWindowWindowName::_1_buffer, MaaStringBufferHandle);

using vec_str_helper_state = std::tuple<std::vector<std::string>, std::vector<MaaStringView>>;
LHGArgOuterState(
    maa::func_type_MaaToolkitRegisterCustomRecognizerExecutor::_3_exec_params,
    vec_str_helper_state);
LHGArgOuterState(
    maa::func_type_MaaToolkitRegisterCustomActionExecutor::_3_exec_params,
    vec_str_helper_state);

LHGArgOuterState(maa::func_type_MaaQueryTaskDetail::_1_node_id_list, std::vector<MaaNodeId>);
LHGArgOuterState(maa::func_type_MaaQueryTaskDetail::_2_node_id_list_size, MaaSize);
