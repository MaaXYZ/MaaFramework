#include <MaaControlUnit/Win32ControlUnitAPI.h>

using namespace MAA_CTRL_UNIT_NS;

int main()
{
    auto* handle = MaaWin32ControlUnitCreate(nullptr, MaaWin32ScreencapMethod_None, MaaWin32InputMethod_None, MaaWin32InputMethod_None);
    MaaControlUnitHandle base = handle;
    bool ok = base && !base->connected() && dynamic_cast<Win32ControlUnitAPI*>(base) == handle && dynamic_cast<ScrollableUnit*>(base);
    MaaWin32ControlUnitDestroy(handle);
    return ok ? 0 : 1;
}
