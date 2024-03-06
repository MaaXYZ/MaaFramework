/**
 * @file MaaSyncContext.h
 * @author
 * @brief SyncContext is a context for running tasks, recognizers, actions, and other operations.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_FRAMEWORK_API
        MaaSyncContextRunTask(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView param);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunRecognizer(
        MaaSyncContextHandle sync_context,
        MaaImageBufferHandle image,
        MaaStringView task_name,
        MaaStringView task_param,
        MaaRectHandle out_box /**< [out] */,
        MaaStringBufferHandle out_detail /**< [out] */);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunAction(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView task_param,
        MaaRectHandle cur_box,
        MaaStringView cur_rec_detail);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextSwipe(
        MaaSyncContextHandle sync_context, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextPressKey(MaaSyncContextHandle sync_context, int32_t keycode);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextInputText(MaaSyncContextHandle sync_context, MaaStringView text);

    MaaBool MAA_FRAMEWORK_API MaaSyncContextTouchDown(
        MaaSyncContextHandle sync_context, int32_t contact, int32_t x, int32_t y, int32_t pressure);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextTouchMove(
        MaaSyncContextHandle sync_context, int32_t contact, int32_t x, int32_t y, int32_t pressure);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextTouchUp(MaaSyncContextHandle sync_context, int32_t contact);

    MaaBool MAA_FRAMEWORK_API
        MaaSyncContextScreencap(MaaSyncContextHandle sync_context, MaaImageBufferHandle out_image /**< [out] */);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextGetTaskResult(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringBufferHandle out_task_result /**< [out] */);

#ifdef __cplusplus
}
#endif
