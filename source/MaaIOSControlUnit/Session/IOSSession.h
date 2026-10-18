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
#include "wifi/PairRecord.h"

namespace maa::ios
{
struct Frame;

namespace media
{
class FramePump;
}

namespace remote
{
class Device;
class DisplayWatcher;
}

namespace hid
{
class Service;
class Buttons;
}
}

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
        MaaIOScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        const std::optional<maa::ios::wifi::PairRecord>& pairing,
        std::string& err);
    void close();

    [[nodiscard]] bool connected() const { return device_ != nullptr; }

    bool screencap(cv::Mat& image, std::string& err);
    bool logical_to_panel(int x, int y, int width, int height, double& fx, double& fy, std::string& err) const;
    bool touch(double x, double y, bool down, std::string& err);
    bool release_touch(std::string& err);

    [[nodiscard]] bool touch_active() const noexcept { return inputs_.touch_active; }

    bool tap(double x, double y, int hold_ms, std::string& err);
    bool stroke(const std::vector<std::pair<double, double>>& points, int step_ms, std::string& err);
    bool key_down(int key, std::string& err);
    bool key_up(int key, std::string& err);
    bool click_key(int key, std::string& err);
    bool type_text(const std::string& text, std::string& err);
    bool release_inputs(std::string& err);

    [[nodiscard]] const std::string& udid() const { return udid_; }

    [[nodiscard]] std::string device_property(const char* key) const;
    bool launch_app(const std::string& bundle_id, std::string& err);
    bool stop_app(const std::string& bundle_id, std::string& err);

private:
    bool prepare_services(std::string& err);
    bool convert(const maa::ios::Frame& frame, cv::Mat& image, std::string& err);
    bool screencap_via_service(cv::Mat& image, std::string& err);
    bool send_keyboard(const std::set<uint16_t>& usages, std::string& err);
    bool ensure_buttons(std::string& err);

    std::unique_ptr<maa::ios::remote::Device> device_;
    std::unique_ptr<maa::ios::remote::DisplayWatcher> watcher_;
    std::unique_ptr<maa::ios::media::FramePump> pump_;
    std::unique_ptr<maa::ios::hid::Service> hid_;
    std::unique_ptr<maa::ios::hid::Buttons> buttons_;
    std::string udid_;
    MaaIOScreencapMethod screencap_methods_ = MaaIOScreencapMethod_Default;
    ImageMapping mapping_;
    HeldInputs inputs_;
};

} // namespace maa::ios_unit
