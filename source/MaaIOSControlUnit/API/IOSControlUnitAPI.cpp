#include "MaaControlUnit/IOSControlUnitAPI.h"
#include "Common/IOSPairing.h"

#include <MaaUtils/Logger.h>

#include "Manager/IOSControlUnitMgr.h"

extern "C"
{
    const char* MaaIOSControlUnitGetVersion()
    {
#pragma message("MaaIOSControlUnit MAA_VERSION: " MAA_VERSION)

        return MAA_VERSION;
    }

    MaaIOSControlUnitHandle MaaIOSControlUnitCreate(
        const char* udid,
        MaaIOSScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing)
    {
        using namespace MAA_CTRL_UNIT_NS;

        // 直接 VAR(udid) 会在 NULL 时把空指针交给 operator<<(const char*)，那是 UB。
        const std::string udid_str = udid ? udid : "";
        const std::string wifi_str = wifi_address ? wifi_address : "";
        LogFunc << VAR(udid_str) << VAR(wifi_str) << VAR(screencap_methods);

        try {
            if (udid_str.empty()) {
                LogError << "iOS device UDID is required";
                return nullptr;
            }
            if (connection_method < MaaIOSConnectionMethod_Auto || connection_method > MaaIOSConnectionMethod_WiFi) {
                LogError << "Invalid iOS connection method";
                return nullptr;
            }
            std::optional<maa::ios::wifi::PairRecord> pairing;
            if (wifi_pairing && connection_method != MaaIOSConnectionMethod_USB) {
                std::string error;
                pairing = maa::ios::wifi::copy_pairing(*wifi_pairing, error);
                if (!pairing || udid_str != pairing->udid) {
                    LogError << "Invalid or mismatched iOS Wi-Fi pairing credentials";
                    return nullptr;
                }
            }
            if (connection_method == MaaIOSConnectionMethod_WiFi && !pairing) {
                LogError << "Wi-Fi requires pairing credentials";
                return nullptr;
            }
            auto unit_mgr =
                std::make_unique<IOSControlUnitMgr>(udid_str, wifi_str, screencap_methods, connection_method, std::move(pairing));
            return unit_mgr.release();
        }
        catch (...) {
            LogError << "Failed to create iOS control unit";
            return nullptr;
        }
    }

    void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle)
    {
        LogFunc << VAR_VOIDP(handle);

        if (handle) {
            delete handle;
        }
    }

} // extern "C"
