#include <array>
#include <atomic>
#include <string>
#include <string_view>

#include <opencv2/core/hal/interface.h>

#include "MaaFramework/MaaAPI.h"
#define MAA_PLUGIN_EXPORTS
#include "MaaPlugin/MaaPluginAPI.h"

namespace
{
std::atomic_int init_count = 0;
std::atomic_int live_instances = 0;
std::atomic_int controller_events = 0;
std::atomic_int context_events = 0;
std::atomic_int misrouted_events = 0;

struct Device
{
    std::string config;
    std::array<uint8_t, 6> pixels { 1, 2, 3, 4, 5, 6 };
};

MaaBool connect(void*)
{
    return true;
}

MaaBool request_uuid(void* arg, MaaStringBuffer* buffer)
{
    return MaaStringBufferSet(buffer, static_cast<Device*>(arg)->config.c_str());
}

MaaBool screencap(void* arg, MaaImageBuffer* buffer)
{
    return MaaImageBufferSetRawData(buffer, static_cast<Device*>(arg)->pixels.data(), 2, 1, CV_8UC3);
}

MaaBool click(int32_t x, int32_t y, void* arg)
{
    if (x != 1 || y != 0) {
        return false;
    }
    ++static_cast<Device*>(arg)->pixels[0];
    return true;
}

MaaBool get_info(void*, MaaStringBuffer* buffer)
{
    std::string info = "{\"live_instances\":" + std::to_string(live_instances.load()) + ",\"controller_events\":"
                       + std::to_string(controller_events.load()) + ",\"context_events\":" + std::to_string(context_events.load())
                       + ",\"misrouted_events\":" + std::to_string(misrouted_events.load()) + "}";
    return MaaStringBufferSet(buffer, info.c_str());
}

void release(void* arg)
{
    delete static_cast<Device*>(arg);
    --live_instances;
}
} // namespace

uint32_t GetApiVersion()
{
    return 1;
}

void OnPluginInit()
{
    ++init_count;
}

const char* GetCustomControllerName()
{
    return "PixelDevice";
}

MaaController* CreateCustomController(const char* config_json)
{
    if (init_count != 1) {
        return nullptr;
    }
    static MaaCustomControllerCallbacks callbacks {
        .connect = connect,
        .request_uuid = request_uuid,
        .screencap = screencap,
        .click = click,
        .get_info = get_info,
    };
    auto* device = new Device { config_json };
    ++live_instances;
    return MaaCustomControllerCreateWithRelease(&callbacks, device, release);
}

void OnControllerEvent(void*, const char* message, const char*, void*)
{
    if (std::string_view(message).starts_with("Controller.")) {
        ++controller_events;
    }
    else {
        ++misrouted_events;
    }
}

void OnContextEvent(void*, const char* message, const char*, void*)
{
    if (std::string_view(message).starts_with("Node.")) {
        ++context_events;
    }
    else if (std::string_view(message).starts_with("Controller.")) {
        ++misrouted_events;
    }
}
