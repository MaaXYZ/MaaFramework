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

    MAA_TOOLKIT_API MaaToolkitGamescopeEisSocketList* MaaToolkitGamescopeEisSocketListCreate();
    MAA_TOOLKIT_API void MaaToolkitGamescopeEisSocketListDestroy(MaaToolkitGamescopeEisSocketList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitGamescopeEisSocketFindAll(/*out*/ MaaToolkitGamescopeEisSocketList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitGamescopeEisSocketListSize(const MaaToolkitGamescopeEisSocketList* list);
    MAA_TOOLKIT_API const MaaToolkitGamescopeEisSocket* MaaToolkitGamescopeEisSocketListAt(const MaaToolkitGamescopeEisSocketList* list, MaaSize index);

    MAA_TOOLKIT_API const char* MaaToolkitGamescopeEisSocketGetPath(const MaaToolkitGamescopeEisSocket* socket);

#ifdef __cplusplus
}
#endif
