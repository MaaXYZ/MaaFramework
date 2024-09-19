/**
 * @file MaaResource.h
 * @author
 * @brief The resource API.
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

    MAA_FRAMEWORK_API MaaResource* MaaResourceCreate(MaaNotificationCallback notify, void* notify_trans_arg);

    MAA_FRAMEWORK_API void MaaResourceDestroy(MaaResource* res);

    MAA_FRAMEWORK_API MaaBool
        MaaResourceRegisterCustomRecognition(MaaResource* res, const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg);

    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomRecognition(MaaResource* res, const char* name);

    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomRecognition(MaaResource* res);

    MAA_FRAMEWORK_API MaaBool
        MaaResourceRegisterCustomAction(MaaResource* res, const char* name, MaaCustomActionCallback action, void* trans_arg);

    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomAction(MaaResource* res, const char* name);

    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomAction(MaaResource* res);

    MAA_FRAMEWORK_API MaaResId MaaResourcePostPath(MaaResource* res, const char* path);

    MAA_FRAMEWORK_API MaaBool MaaResourceClear(MaaResource* res);

    MAA_FRAMEWORK_API MaaStatus MaaResourceStatus(const MaaResource* res, MaaResId id);

    MAA_FRAMEWORK_API MaaStatus MaaResourceWait(const MaaResource* res, MaaResId id);

    MAA_FRAMEWORK_API MaaBool MaaResourceLoaded(const MaaResource* res);

    MAA_FRAMEWORK_API MaaBool MaaResourceSetOption(MaaResource* res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetHash(const MaaResource* res, /* out */ MaaStringBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetTaskList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer);

#ifdef __cplusplus
}
#endif
