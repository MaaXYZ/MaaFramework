#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <opencv2/core/mat.hpp>

#include "HeldInputs.h"
#include "InputMapping.h"
#include "MaaFramework/MaaDef.h"
#include "XdbClient.h"

namespace maa::ios_unit
{

// Owns one device connection and its selected image/input services.
class IOSSession
{
public:
    IOSSession();
    ~IOSSession();
    IOSSession(const IOSSession&) = delete;
    IOSSession& operator=(const IOSSession&) = delete;

    bool create(
        const std::string& udid,
        const std::string& wifi_address,
        MaaIOSScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        const std::string& config,
        std::string& err);
    void close();

    [[nodiscard]] bool connected() const { return client_.connected(); }

    bool screencap(cv::Mat& image, std::string& err);
    bool logical_to_panel(int x, int y, int width, int height, double& fx, double& fy, std::string& err) const;
    bool touch(int contact, double x, double y, bool down, std::string& err);
    bool release_touch(int contact, std::string& err);

    [[nodiscard]] bool touch_active(int contact) const noexcept { return inputs_.touches.contains(contact); }

    bool key_down(int key, std::string& err);
    bool key_up(int key, std::string& err);
    bool type_text(const std::string& text, std::string& err);
    bool release_inputs(std::string& err);

    [[nodiscard]] const std::string& udid() const { return udid_; }

    [[nodiscard]] std::string device_property(const char* key) const;
    bool launch_app(const std::string& bundle_id, std::string& err);
    bool stop_app(const std::string& bundle_id, std::string& err);

private:
    bool convert(const json::value& frame, const std::vector<uint8_t>& pixels, cv::Mat& image, std::string& err);
    bool screencap_via_service(cv::Mat& image, std::string& err);
    bool send_keyboard(const std::set<uint16_t>& usages, std::string& err);

    XdbClient client_;
    json::value properties_;
    bool video_ = false;
    uint64_t serial_ = 0;
    std::string udid_;
    MaaIOSScreencapMethod screencap_methods_ = MaaIOSScreencapMethod_Default;
    ImageMapping mapping_;
    HeldInputs inputs_;
};

} // namespace maa::ios_unit
