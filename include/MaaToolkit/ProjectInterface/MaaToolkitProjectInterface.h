// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API void MaaToolkitProjectInterfaceRegisterCustomRecognition(
        uint64_t inst_id,
        const char* name,
        MaaCustomRecognitionCallback recognition,
        void* trans_arg);
    MAA_TOOLKIT_API void
        MaaToolkitProjectInterfaceRegisterCustomAction(uint64_t inst_id, const char* name, MaaCustomActionCallback action, void* trans_arg);

    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceRunCli(
        uint64_t inst_id,
        const char* resource_path,
        const char* user_path,
        MaaBool directly,
        MaaNotificationCallback notify,
        void* notify_trans_arg);

#ifdef __cplusplus
}
#endif
