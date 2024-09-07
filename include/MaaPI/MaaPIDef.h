#pragma once

#include "MaaFramework/MaaDef.h" // IWYU pragma: export

struct MaaPIData;
struct MaaPIConfig;
struct MaaPIRuntime;
struct MaaPIClient;

typedef int32_t MaaPIClientAction;

enum MaaPIClientAction_Enum
{
    MaaPIClientAction_SelectController,
    MaaPIClientAction_SelectResource,
    MaaPIClientAction_AddTask,
    MaaPIClientAction_MoveTask,
    MaaPIClientAction_RemoveTask,
    MaaPIClientAction_ConfigureTask,
    MaaPIClientAction_LaunchTask,

    MaaPIClientAction_Derived_SelectAdb,
    MaaPIClientAction_Derived_SelectDesktop,
    MaaPIClientAction_Derived_SelectTask,
    MaaPIClientAction_Derived_SelectOption,

    MaaPIClientAction_DirectBegin = MaaPIClientAction_SelectController,
    MaaPIClientAction_DirectEnd = MaaPIClientAction_LaunchTask,
    MaaPIClientAction_DerivedBegin = MaaPIClientAction_Derived_SelectAdb,
    MaaPIClientAction_DerivedEnd = MaaPIClientAction_Derived_SelectOption,
};

typedef int32_t (*MaaPIClientHandler)(
    MaaPIClient* client,
    MaaPIClientAction action,
    MaaStringListBuffer* choice,
    MaaStringListBuffer* details,
    void* arg);
