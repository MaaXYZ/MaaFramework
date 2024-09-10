// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API void
        MaaToolkitProjectInterfaceRegisterCustomRecognition(const char* name, MaaCustomRecognizerCallback recognizer, void* trans_arg);
    MAA_TOOLKIT_API void MaaToolkitProjectInterfaceRegisterCustomAction(const char* name, MaaCustomActionCallback action, void* trans_arg);

    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceRunCli(
        const char* resource_path,
        const char* user_path,
        MaaBool directly,
        MaaNotificationCallback callback,
        void* callback_arg);

#ifdef __cplusplus
}
#endif
