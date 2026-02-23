#include "ControlUnit/ControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/MacOSControlUnitMgr.h"

extern "C"
{
    const char* MaaMacOSControlUnitGetVersion()
    {
#pragma message("MaaMacOSControlUnit MAA_VERSION: " MAA_VERSION)

        return MAA_VERSION;
    }

    MaaMacOSControlUnitHandle
        MaaMacOSControlUnitCreate(uint32_t window_id, MaaMacOSScreencapMethod screencap_method, MaaMacOSInputMethod input_method)
    {
        using namespace MAA_CTRL_UNIT_NS;

        LogFunc << VAR(window_id) << VAR(screencap_method) << VAR(input_method);

        auto unit_mgr = std::make_unique<MacOSControlUnitMgr>(window_id, screencap_method, input_method);
        return unit_mgr.release();
    }

    void MaaMacOSControlUnitDestroy(MaaMacOSControlUnitHandle handle)
    {
        LogFunc << VAR_VOIDP(handle);

        if (handle) {
            delete handle;
        }
    }

} // extern "C"
