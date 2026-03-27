#pragma once

#include "MaaControlUnitDef.h" // IWYU pragma: export

#include "ControlUnitAPI.h"
#include "RecordTypes.h"

#include "AdbControlUnitAPI.h"
#ifdef __ANDROID__
#include "AndroidNativeControlUnitAPI.h"
#endif
#include "CustomControlUnitAPI.h"
#include "DbgControlUnitAPI.h"
#include "GamepadControlUnitAPI.h"
#include "MacOSControlUnitAPI.h"
#include "PlayCoverControlUnitAPI.h"
#include "RecordControlUnitAPI.h"
#include "ReplayControlUnitAPI.h"
#include "Win32ControlUnitAPI.h"
#include "WlRootsControlUnitAPI.h"
