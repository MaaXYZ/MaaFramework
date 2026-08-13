// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaToolkitGamescopeNodeList* MaaToolkitGamescopeNodeListCreate();
    MAA_TOOLKIT_API void MaaToolkitGamescopeNodeListDestroy(MaaToolkitGamescopeNodeList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitGamescopeNodeFindAll(/*out*/ MaaToolkitGamescopeNodeList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitGamescopeNodeListSize(const MaaToolkitGamescopeNodeList* list);
    MAA_TOOLKIT_API const MaaToolkitGamescopeNode* MaaToolkitGamescopeNodeListAt(const MaaToolkitGamescopeNodeList* list, MaaSize index);

    MAA_TOOLKIT_API const char* MaaToolkitGamescopeNodeGetName(const MaaToolkitGamescopeNode* node);
    MAA_TOOLKIT_API uint32_t MaaToolkitGamescopeNodeGetId(const MaaToolkitGamescopeNode* node);

#ifdef __cplusplus
}
#endif
