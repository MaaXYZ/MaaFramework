#pragma once

#include "MaaFramework/MaaAPI.h"

#if defined(LHG_PROCESS) && !defined(LHG_BUILD)
#define FAKE_IMPORT MAA_FRAMEWORK_API
#else
#define FAKE_IMPORT
#endif

MaaBool FAKE_IMPORT
    MaaControllerSetOptionString(MaaControllerHandle ctrl, MaaCtrlOption key, MaaStringView value);
MaaBool FAKE_IMPORT
    MaaControllerSetOptionInteger(MaaControllerHandle ctrl, MaaCtrlOption key, int value);
MaaBool FAKE_IMPORT
    MaaControllerSetOptionBoolean(MaaControllerHandle ctrl, MaaCtrlOption key, bool value);

MaaBool FAKE_IMPORT MaaSetGlobalOptionString(MaaCtrlOption key, MaaStringView value);
MaaBool FAKE_IMPORT MaaSetGlobalOptionInteger(MaaCtrlOption key, int value);
MaaBool FAKE_IMPORT MaaSetGlobalOptionBoolean(MaaCtrlOption key, bool value);
