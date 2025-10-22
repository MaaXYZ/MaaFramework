#pragma once

#include <MaaFramework/MaaAPI.h>

void ResourceSink(void* controller, const char* message, const char* details_json, void* callback_arg);
void ControllerSink(void* controller, const char* message, const char* details_json, void* callback_arg);
void TaskerSink(void* controller, const char* message, const char* details_json, void* callback_arg);
void ContextSink(void* controller, const char* message, const char* details_json, void* callback_arg);
MaaBool CustomReco(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    MaaRect* out_box,
    MaaStringBuffer* out_detail);
MaaBool CustomAct(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg);
