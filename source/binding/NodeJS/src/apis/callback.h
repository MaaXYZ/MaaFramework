#pragma once

#include <MaaFramework/MaaAPI.h>

void ResourceSink(void* resource, const char* message, const char* details_json, void* callback_arg);
void ControllerSink(void* controller, const char* message, const char* details_json, void* callback_arg);
void TaskerSink(void* tasker, const char* message, const char* details_json, void* callback_arg);
void ContextSink(void* context, const char* message, const char* details_json, void* callback_arg);

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

MaaBool CustomConnect(void* trans_arg);
MaaBool CustomConnected(void* trans_arg);
MaaBool CustomRequestUuid(void* trans_arg, MaaStringBuffer* buffer);
MaaControllerFeature CustomGetFeatures(void* trans_arg);
MaaBool CustomStartApp(const char* intent, void* trans_arg);
MaaBool CustomStopApp(const char* intent, void* trans_arg);
MaaBool CustomScreencap(void* trans_arg, MaaImageBuffer* buffer);
MaaBool CustomClick(int32_t x, int32_t y, void* trans_arg);
MaaBool CustomSwipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, void* trans_arg);
MaaBool CustomTouchDown(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg);
MaaBool CustomTouchMove(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg);
MaaBool CustomTouchUp(int32_t contact, void* trans_arg);
MaaBool CustomClickKey(int32_t keycode, void* trans_arg);
MaaBool CustomInputText(const char* text, void* trans_arg);
MaaBool CustomKeyDown(int32_t keycode, void* trans_arg);
MaaBool CustomKeyUp(int32_t keycode, void* trans_arg);

inline MaaCustomControllerCallbacks CustomCallbacks {
    .connect = CustomConnect,
    .connected = CustomConnected,
    .request_uuid = CustomRequestUuid,
    .get_features = CustomGetFeatures,
    .start_app = CustomStartApp,
    .stop_app = CustomStopApp,
    .screencap = CustomScreencap,
    .click = CustomClick,
    .swipe = CustomSwipe,
    .touch_down = CustomTouchDown,
    .touch_move = CustomTouchMove,
    .touch_up = CustomTouchUp,
    .click_key = CustomClickKey,
    .input_text = CustomInputText,
    .key_down = CustomKeyDown,
    .key_up = CustomKeyUp,
};
