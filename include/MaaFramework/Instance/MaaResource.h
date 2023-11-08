#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaResourceHandle MAA_FRAMEWORK_API MaaResourceCreate(MaaResourceCallback callback,
                                                          MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaResourceDestroy(MaaResourceHandle res);

    MaaResId MAA_FRAMEWORK_API MaaResourcePostPath(MaaResourceHandle res, MaaStringView path);
    MaaStatus MAA_FRAMEWORK_API MaaResourceStatus(MaaResourceHandle res, MaaResId id);
    MaaStatus MAA_FRAMEWORK_API MaaResourceWait(MaaResourceHandle res, MaaResId id);
    MaaBool MAA_FRAMEWORK_API MaaResourceLoaded(MaaResourceHandle res);

    MaaBool MAA_FRAMEWORK_API MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value,
                                                   MaaOptionValueSize val_size);
    MaaBool MAA_FRAMEWORK_API MaaResourceGetHash(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer);
    MaaBool MAA_FRAMEWORK_API MaaResourceGetTaskList(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
