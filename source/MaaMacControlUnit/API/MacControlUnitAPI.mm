#include "MaaFramework/MaaDef.h"
#import <Cocoa/Cocoa.h>
#include <memory>

#include "ControlUnit/MacControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Manager/MacControlUnitMgr.h"
#include "Utils/Logger.h"

const char* MaaMacControlUnitGetVersion()
{
#pragma message("MaaMacControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaMacControlUnitCreate(uint32_t windowId, MaaMacScreencapMethod screencap_method, MaaMacInputMethod input_method)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(windowId) << VAR(screencap_method) << VAR(input_method);

    auto unit_mgr = std::make_unique<MacControlUnitMgr>(windowId, screencap_method, input_method);
    return unit_mgr.release();
}

void MaaMacControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
