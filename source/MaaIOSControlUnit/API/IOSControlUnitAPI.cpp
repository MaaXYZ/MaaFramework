#include "MaaControlUnit/IOSControlUnitAPI.h"
#include "Manager/IOSControlUnitMgr.h"
#include <MaaUtils/Logger.h>

extern "C" {
const char* MaaIOSControlUnitGetVersion() { return MAA_VERSION; }
MaaIOSControlUnitHandle MaaIOSControlUnitCreate(const char* udid, MaaIOSScreencapMethod methods,
    MaaIOSConnectionMethod connection, const char* config)
{
    if (!udid || !*udid || connection < MaaIOSConnectionMethod_Auto || connection > MaaIOSConnectionMethod_WiFi) return nullptr;
    try {
        return new MAA_CTRL_UNIT_NS::IOSControlUnitMgr(udid, methods, connection, config ? config : "{}");
    } catch (...) { LogError << "Failed to create iOS controller"; return nullptr; }
}
void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle) { delete handle; }
}
