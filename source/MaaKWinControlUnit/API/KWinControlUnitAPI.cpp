#include <opencv2/core.hpp>

#include "MaaControlUnit/KWinControlUnitAPI.h"

#include "Manager/KWinControlUnitMgr.h"
#include "MaaUtils/Logger.h"

const char* MaaKWinControlUnitGetVersion()
{
    return MAA_VERSION;
}

MaaKWinControlUnitHandle MaaKWinControlUnitCreate(const char* device_node, int screen_width, int screen_height)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(device_node) << VAR(screen_width) << VAR(screen_height);

    if (!device_node || device_node[0] == '\0') {
        LogError << "device_node is null or empty";
        return nullptr;
    }

    if (screen_width <= 0 || screen_height <= 0) {
        LogError << "Invalid screen dimensions" << VAR(screen_width) << VAR(screen_height);
        return nullptr;
    }

    auto unit_mgr = std::make_unique<KWinControlUnitMgr>(device_node, screen_width, screen_height);
    return unit_mgr.release();
}

MaaBool MaaKWinControlUnitConnect(MaaKWinControlUnitHandle handle)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(handle);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    return handle->connect();
}

MaaBool MaaKWinControlUnitTestScreencap(MaaKWinControlUnitHandle handle)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(handle);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat image;
    bool ok = handle->screencap(image);
    if (ok) {
        LogInfo << "screencap succeeded, image size=" << image.size();
    }
    else {
        LogError << "screencap failed";
    }
    return ok;
}

void MaaKWinControlUnitDestroy(MaaKWinControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}
