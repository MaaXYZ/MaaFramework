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

    MAA_FRAMEWORK_API MaaTaskId MaaSyncContextRunTask(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView param);

    MAA_FRAMEWORK_API MaaTaskId MaaSyncContextRunRecognition(
        MaaSyncContextHandle sync_context,
        MaaImageBufferHandle image,
        MaaStringView task_name,
        MaaStringView task_param);

    MAA_FRAMEWORK_API MaaTaskId MaaSyncContextRunAction(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView task_param,
        MaaRectHandle cur_box,
        MaaStringView cur_rec_detail);

    MAA_FRAMEWORK_API MaaInstanceHandle MaaSyncContextGetInstance(MaaSyncContextHandle sync_context);

#ifdef __cplusplus
}
#endif
