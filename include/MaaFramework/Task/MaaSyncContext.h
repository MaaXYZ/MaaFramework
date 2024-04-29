/**
 * @file MaaSyncContext.h
 * @author
 * @brief SyncContext is a context for running tasks, recognizers, actions, and other operations.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_FRAMEWORK_API MaaBool MaaSyncContextRunTask(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView param);
    MAA_FRAMEWORK_API MaaBool MaaSyncContextRunRecognition(
        MaaSyncContextHandle sync_context,
        MaaImageBufferHandle image,
        MaaStringView task_name,
        MaaStringView task_param,
        MaaRectHandle out_box /**< [out] */,
        MaaStringBufferHandle out_detail /**< [out] */);
    MAA_FRAMEWORK_API MaaBool MaaSyncContextRunAction(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView task_param,
        MaaRectHandle cur_box,
        MaaStringView cur_rec_detail);

    MAA_FRAMEWORK_API MaaBool
        MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y);
    MAA_FRAMEWORK_API MaaBool MaaSyncContextSwipe(
        MaaSyncContextHandle sync_context,
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        int32_t duration);
    MAA_FRAMEWORK_API MaaBool
        MaaSyncContextPressKey(MaaSyncContextHandle sync_context, int32_t keycode);
    MAA_FRAMEWORK_API MaaBool
        MaaSyncContextInputText(MaaSyncContextHandle sync_context, MaaStringView text);

    MAA_FRAMEWORK_API MaaBool MaaSyncContextTouchDown(
        MaaSyncContextHandle sync_context,
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure);
    MAA_FRAMEWORK_API MaaBool MaaSyncContextTouchMove(
        MaaSyncContextHandle sync_context,
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure);
    MAA_FRAMEWORK_API MaaBool
        MaaSyncContextTouchUp(MaaSyncContextHandle sync_context, int32_t contact);

    MAA_FRAMEWORK_API MaaBool MaaSyncContextScreencap(
        MaaSyncContextHandle sync_context,
        MaaImageBufferHandle out_image /**< [out] */);

    MAA_FRAMEWORK_API MaaBool MaaSyncContextCachedImage(
        MaaSyncContextHandle sync_context,
        MaaImageBufferHandle out_image /**< [out] */);

#ifdef __cplusplus
}
#endif
