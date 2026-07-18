// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MAA_TOOLKIT_API MaaToolkitPortalHelper* MaaToolkitPortalHelperCreate();
    MAA_TOOLKIT_API void MaaToolkitPortalHelperDestroy(MaaToolkitPortalHelper* helper);
    MAA_TOOLKIT_API bool MaaToolkitPortalHelperOpenStream(MaaToolkitPortalHelper* helper);
    MAA_TOOLKIT_API int MaaToolkitPortalHelperGetPipeWireFD(MaaToolkitPortalHelper* helper);
    MAA_TOOLKIT_API uint32_t MaaToolkitPortalHelperGetPipeWireNodeID(MaaToolkitPortalHelper* helper);
#ifdef __cplusplus
}
#endif
