#include <cstring>

#include "MaaFramework/Instance/MaaInstance.h"
#include "MaaFramework/MaaDef.h"
#include "extra.h"

MaaBool
    MaaControllerSetOptionString(MaaControllerHandle ctrl, MaaCtrlOption key, MaaStringView value)
{
    return MaaControllerSetOption(ctrl, key, const_cast<char*>(value), strlen(value));
}

MaaBool MaaControllerSetOptionInteger(MaaControllerHandle ctrl, MaaCtrlOption key, int value)
{
    return MaaControllerSetOption(ctrl, key, &value, 4);
}

MaaBool MaaControllerSetOptionBoolean(MaaControllerHandle ctrl, MaaCtrlOption key, bool value)
{
    MaaBool v = !!value;
    return MaaControllerSetOption(ctrl, key, &v, 1);
}

MaaBool MaaSetGlobalOptionString(MaaCtrlOption key, MaaStringView value)
{
    return MaaSetGlobalOption(key, const_cast<char*>(value), strlen(value));
}

MaaBool MaaSetGlobalOptionInteger(MaaCtrlOption key, int value)
{
    return MaaSetGlobalOption(key, &value, 4);
}

MaaBool MaaSetGlobalOptionBoolean(MaaCtrlOption key, bool value)
{
    MaaBool v = !!value;
    return MaaSetGlobalOption(key, &v, 1);
}
