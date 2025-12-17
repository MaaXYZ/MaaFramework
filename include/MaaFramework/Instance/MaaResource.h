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

    MAA_FRAMEWORK_API MaaResource* MaaResourceCreate();

    MAA_FRAMEWORK_API void MaaResourceDestroy(MaaResource* res);

    MAA_FRAMEWORK_API MaaSinkId MaaResourceAddSink(MaaResource* res, MaaEventCallback sink, void* trans_arg);

    MAA_FRAMEWORK_API void MaaResourceRemoveSink(MaaResource* res, MaaSinkId sink_id);

    MAA_FRAMEWORK_API void MaaResourceClearSinks(MaaResource* res);

    MAA_FRAMEWORK_API MaaBool
        MaaResourceRegisterCustomRecognition(MaaResource* res, const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg);

    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomRecognition(MaaResource* res, const char* name);

    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomRecognition(MaaResource* res);

    MAA_FRAMEWORK_API MaaBool
        MaaResourceRegisterCustomAction(MaaResource* res, const char* name, MaaCustomActionCallback action, void* trans_arg);

    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomAction(MaaResource* res, const char* name);

    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomAction(MaaResource* res);

    MAA_FRAMEWORK_API MaaResId MaaResourcePostBundle(MaaResource* res, const char* path);

    MAA_FRAMEWORK_API MaaResId MaaResourcePostOcrModel(MaaResource* res, const char* path);

    MAA_FRAMEWORK_API MaaResId MaaResourcePostPipeline(MaaResource* res, const char* path);

    MAA_FRAMEWORK_API MaaResId MaaResourcePostImage(MaaResource* res, const char* path);

    MAA_FRAMEWORK_API MaaBool MaaResourceOverridePipeline(MaaResource* res, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaBool MaaResourceOverrideNext(MaaResource* res, const char* node_name, const MaaStringListBuffer* next_list);

    MAA_FRAMEWORK_API MaaBool MaaResourceOverrideImage(MaaResource* res, const char* image_name, const MaaImageBuffer* image);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetNodeData(MaaResource* res, const char* node_name, /* out */ MaaStringBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaResourceClear(MaaResource* res);

    MAA_FRAMEWORK_API MaaStatus MaaResourceStatus(const MaaResource* res, MaaResId id);

    MAA_FRAMEWORK_API MaaStatus MaaResourceWait(const MaaResource* res, MaaResId id);

    MAA_FRAMEWORK_API MaaBool MaaResourceLoaded(const MaaResource* res);

    MAA_FRAMEWORK_API MaaBool MaaResourceSetOption(MaaResource* res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetHash(const MaaResource* res, /* out */ MaaStringBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetNodeList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetCustomRecognitionList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaResourceGetCustomActionList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer);

#ifdef __cplusplus
}
#endif
