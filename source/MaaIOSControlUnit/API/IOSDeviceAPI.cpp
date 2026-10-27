#include "ControlUnit/IOSDeviceAPI.h"
#include "MaaUtils/Buffer/BufferTypes.hpp"
#include "Session/XdbClient.h"
#include <atomic>
#include <thread>

namespace
{
class CancellationMonitor
{
public:
    CancellationMonitor(maa::ios_unit::XdbClient& client, MaaToolkitIOSDeviceCancelCallback cancel, void* userdata)
        : worker_([this, &client, cancel, userdata] {
            while (!stopped_.load()) {
                if (cancel && cancel(userdata)) {
                    client.interrupt();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        })
    {
    }

    ~CancellationMonitor() { stop(); }

    void stop()
    {
        stopped_ = true;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

private:
    std::atomic<bool> stopped_ { false };
    std::thread worker_;
};

class DeviceAPI final : public MAA_CTRL_UNIT_NS::IOSDeviceAPI
{
public:
    MaaBool discover(
        MaaIOSConnectionMethod mode,
        const char* options,
        MaaToolkitIOSDeviceEventCallback,
        MaaToolkitIOSDeviceCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result) override
    {
        if (!result) {
            return false;
        }
        if (cancel && cancel(userdata)) {
            result->set(R"({"success":false,"cancelled":true,"error":"Discovery cancelled"})");
            return false;
        }
        std::string error;
        json::value value;
        std::vector<uint8_t> data;
        bool ok = false;
        try {
            auto config = json::parse(options && *options ? options : "{}");
            if (!config || !config->is_object() || mode < MaaIOSConnectionMethod_Auto || mode > MaaIOSConnectionMethod_WiFi) {
                error = "Invalid iOS discovery options";
            }
            else {
                maa::ios_unit::XdbClient client;
                if (client.connect(config->to_string(), error)) {
                    CancellationMonitor monitor(client, cancel, userdata);
                    const char* transport = mode == MaaIOSConnectionMethod_USB    ? "usb"
                                            : mode == MaaIOSConnectionMethod_WiFi ? "wifi"
                                                                                  : "auto";
                    ok = client.call(
                        "devices",
                        json::object { { "transport", transport }, { "timeout_ms", config->get("timeout_ms", int64_t(3000)) } },
                        value,
                        data,
                        error);
                    monitor.stop();
                }
            }
            if (ok) {
                auto devices = value.get("devices", json::array { });
                for (auto& device : devices) {
                    device["screencap_methods"] = MaaIOSScreencapMethod_ScreenshotService
                                                  | (value.get("capabilities", "video_decode", false) ? MaaIOSScreencapMethod_Stream : 0);
                }
                value["devices"] = std::move(devices);
            }
        }
        catch (...) {
            error = "iOS discovery failed";
        }
        if (cancel && cancel(userdata)) {
            ok = false;
            error = "Discovery cancelled";
        }
        if (!ok) {
            value = json::object { };
        }
        value["success"] = ok;
        value["cancelled"] = cancel && cancel(userdata);
        value["error"] = error;
        result->set(value.to_string());
        return ok;
    }
};
}

extern "C" MAA_CONTROL_UNIT_API MAA_CTRL_UNIT_NS::IOSDeviceAPI* MaaIOSControlUnitGetDeviceAPI()
{
    static DeviceAPI api;
    return &api;
}
