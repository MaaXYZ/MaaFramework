#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaControlUnit/IOSDeviceAPI.h"

const char* MaaControlUnitGetVersion()
{
    return MAA_VERSION;
}

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitAPI::~ControlUnitAPI() = default;
BindableUnit::~BindableUnit() = default;
ScrollableUnit::~ScrollableUnit() = default;
RelativeMovableUnit::~RelativeMovableUnit() = default;
ShellableUnit::~ShellableUnit() = default;

AdbControlUnitAPI::~AdbControlUnitAPI() = default;
IOSControlUnitAPI::~IOSControlUnitAPI() = default;
IOSDeviceAPI::~IOSDeviceAPI() = default;
AndroidNativeControlUnitAPI::~AndroidNativeControlUnitAPI() = default;
Win32ControlUnitAPI::~Win32ControlUnitAPI() = default;
MacOSControlUnitAPI::~MacOSControlUnitAPI() = default;
WlRootsControlUnitAPI::~WlRootsControlUnitAPI() = default;
KWinControlUnitAPI::~KWinControlUnitAPI() = default;
LinuxControlUnitAPI::~LinuxControlUnitAPI() = default;
CustomControlUnitAPI::~CustomControlUnitAPI() = default;
GamepadControlUnitAPI::~GamepadControlUnitAPI() = default;
FullControlUnitAPI::~FullControlUnitAPI() = default;

MAA_CTRL_UNIT_NS_END
