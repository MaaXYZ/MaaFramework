// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaToolkitGamescopeInstanceList* MaaToolkitGamescopeInstanceListCreate();
    MAA_TOOLKIT_API void MaaToolkitGamescopeInstanceListDestroy(MaaToolkitGamescopeInstanceList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitGamescopeInstanceFindAll(/*out*/ MaaToolkitGamescopeInstanceList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitGamescopeInstanceListSize(const MaaToolkitGamescopeInstanceList* list);
    MAA_TOOLKIT_API const MaaToolkitGamescopeInstance*
        MaaToolkitGamescopeInstanceListAt(const MaaToolkitGamescopeInstanceList* list, MaaSize index);

    MAA_TOOLKIT_API uint32_t MaaToolkitGamescopeInstanceGetDisplayNo(const MaaToolkitGamescopeInstance* instance);
    MAA_TOOLKIT_API uint32_t MaaToolkitGamescopeInstanceGetPipeWireNodeId(const MaaToolkitGamescopeInstance* instance);
    MAA_TOOLKIT_API const char* MaaToolkitGamescopeInstanceGetEisSocketPath(const MaaToolkitGamescopeInstance* instance);

#ifdef __cplusplus
}
#endif
