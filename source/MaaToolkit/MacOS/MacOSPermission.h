#pragma once

#include "Common/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"

MAA_TOOLKIT_NS_BEGIN

bool MacOSCheckPermission(MaaMacOSPermission perm);
bool MacOSRequestPermission(MaaMacOSPermission perm);
bool MacOSRevealPermissionSettings(MaaMacOSPermission perm);

MAA_TOOLKIT_NS_END
