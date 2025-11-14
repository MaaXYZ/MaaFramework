#include "ControlUnit/CustomControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/CustomControlUnitMgr.h"

const char* MaaCustomControlUnitGetVersion()
{
#pragma message("MaaCustomControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaCustomControlUnitHandle MaaCustomControlUnitCreate(MaaCustomControllerCallbacks* controller, void* controller_arg)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(controller) << VAR_VOIDP(controller_arg);

    if (!controller) {
        LogError << "Controller is null";
        return nullptr;
    }

    auto unit_mgr = std::make_unique<CustomControlUnitMgr>(controller, controller_arg);

    return unit_mgr.release();
}

void MaaCustomControlUnitDestroy(MaaCustomControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
