#include "IOSSession.h"
#include <limits>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <utility>

#include <MaaUtils/Logger.h>

namespace maa::ios_unit
{
namespace
{
void rotate_image(cv::Mat& image, int degrees)
{
    switch (degrees) {
    case 90:
        cv::rotate(image, image, cv::ROTATE_90_CLOCKWISE);
        break;
    case 180:
        cv::rotate(image, image, cv::ROTATE_180);
        break;
    case 270:
        cv::rotate(image, image, cv::ROTATE_90_COUNTERCLOCKWISE);
        break;
    default:
        break;
    }
}
}

IOSSession::IOSSession() = default;

IOSSession::~IOSSession()
{
    close();
}

bool IOSSession::create(
    const std::string& udid,
    MaaIOSScreencapMethod methods,
    MaaIOSConnectionMethod connection,
    const std::string& config,
    std::string& err)
{
    close();
    screencap_methods_ = methods & MaaIOSScreencapMethod_All;
    if (!screencap_methods_) {
        err = "No iOS screencap method selected";
        return false;
    }
    const auto options = json::parse(config.empty() ? "{}" : config);
    if (!options || !options->is_object()) {
        err = "Invalid iOS controller configuration";
        return false;
    }
    std::string wifi;
    if (const auto* value = options->find_value("wifi_address")) {
        if (!value->is_string() || value->as_string().find('\0') != std::string::npos) {
            err = "iOS wifi_address must be a string without NUL";
            return false;
        }
        wifi = value->as_string();
    }
    int wifi_port = 49152;
    if (const auto* value = options->find_value("wifi_port")) {
        if (!value->is<int>() || value->as_integer() < 1 || value->as_integer() > 65535) {
            err = "iOS wifi_port must be an integer in [1,65535]";
            return false;
        }
        wifi_port = value->as_integer();
    }
    if (!client_.connect(config, err)) {
        return false;
    }
    json::value result;
    std::vector<uint8_t> data;
    const char* transport = connection == MaaIOSConnectionMethod_USB ? "usb" : connection == MaaIOSConnectionMethod_WiFi ? "wifi" : "auto";
    json::object params { { "udid", udid }, { "wifi", wifi }, { "transport", transport }, { "port", wifi_port } };
    if (!client_.call("connection.open", params, result, data, err)) {
        close();
        return false;
    }
    udid_ = udid;
    if (!client_.call("device.info", json::object { }, properties_, data, err)) {
        close();
        return false;
    }
    // Prewarm geometry for the first capture; capture requests can retry initialization.
    if (!client_.call("display.info", json::object { }, err)) {
        LogWarn << "iOS display geometry prewarm failed" << VAR(err);
    }
    if (methods & MaaIOSScreencapMethod_Stream) {
        // Let the server own stream recovery, including complete receive silence.
        video_ = client_.call("video.start", json::object { }, err);
        if (!video_ && !(methods & MaaIOSScreencapMethod_ScreenshotService)) {
            close();
            return false;
        }
    }
    err.clear();
    return true;
}

void IOSSession::close()
{
    if (client_.connected()) {
        std::string ignored;
        release_inputs(ignored);
        client_.call("connection.close", json::object { }, ignored);
        client_.close();
    }
    udid_.clear();
    properties_ = json::object { };
    mapping_ = { };
    inputs_ = { };
    serial_ = 0;
    video_ = false;
}

bool IOSSession::convert(const json::value& frame, const std::vector<uint8_t>& pixels, cv::Mat& image, std::string& err)
{
    const int width = frame.get("width", 0), height = frame.get("height", 0), pitch = frame.get("row_pitch", 0);
    if (width <= 0 || height <= 0 || int64_t(pitch) < int64_t(width) * 4 || pixels.size() != uint64_t(pitch) * height) {
        err = "Invalid xdb video frame";
        return false;
    }
    auto display = frame.get("display", json::value(json::object { }));
    const int crop_w = display.get("width", 0), crop_h = display.get("height", 0);
    if (crop_w <= 0 || crop_h <= 0 || crop_w > width || crop_h > height) {
        err = "Missing or invalid xdb frame geometry";
        return false;
    }
    const auto angle = orientation_degrees(display.get("orientation", std::string()));
    cv::Mat bgra(height, width, CV_8UC4, const_cast<uint8_t*>(pixels.data()), pitch);
    cv::cvtColor(bgra(cv::Rect(0, 0, crop_w, crop_h)), image, cv::COLOR_BGRA2BGR);
    rotate_image(image, angle.value_or(0));
    mapping_ = { crop_w, crop_h, angle.value_or(0), angle.has_value() };
    return true;
}

bool IOSSession::screencap(cv::Mat& image, std::string& err)
{
    json::value frame;
    std::vector<uint8_t> pixels;
    if (video_
        && client_.call(
            "video.frame",
            json::object { { "since", serial_ }, { "timeout_ms", 300 }, { "allow_cached", true } },
            frame,
            pixels,
            err)) {
        serial_ = frame.get("serial", uint64_t(0));
        if (convert(frame, pixels, image, err)) {
            return true;
        }
    }
    if (!(screencap_methods_ & MaaIOSScreencapMethod_ScreenshotService)) {
        return false;
    }
    return screencap_via_service(image, err);
}

bool IOSSession::screencap_via_service(cv::Mat& image, std::string& err)
{
    json::value frame;
    std::vector<uint8_t> png;
    if (!client_.call("screenshot", json::object { }, frame, png, err) || png.empty() || png.size() > INT_MAX) {
        return false;
    }
    auto decoded = cv::imdecode(png, cv::IMREAD_COLOR);
    if (decoded.empty()) {
        err = "Invalid xdb screenshot PNG";
        return false;
    }
    auto display = frame.get("display", json::value(json::object { }));
    const auto angle = orientation_degrees(display.get("orientation", std::string()));
    const bool swapped = angle && (*angle == 90 || *angle == 270);
    ImageMapping mapping { display.get("width", swapped ? decoded.rows : decoded.cols),
                           display.get("height", swapped ? decoded.cols : decoded.rows),
                           angle.value_or(0),
                           angle.has_value() };
    mapping.valid = mapping.valid && decoded.cols == (swapped ? mapping.height : mapping.width)
                    && decoded.rows == (swapped ? mapping.width : mapping.height);
    mapping_ = mapping;
    image = std::move(decoded);
    return true;
}

bool IOSSession::logical_to_panel(int x, int y, int width, int height, double& fx, double& fy, std::string& err) const
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    if (!mapping_.to_panel(x, y, width, height, fx, fy)) {
        err = "no valid geometry for the delivered screenshot; capture again before input";
        return false;
    }
    return true;
}

bool IOSSession::touch(int contact, double x, double y, bool down, std::string& err)
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    if (contact < 0 || contact >= kMaxTouchContacts) {
        err = "iOS touch contact must be in [0,4]";
        return false;
    }
    if (down) {
        inputs_.touches[contact] = { x, y }; // Failed sends may already have reached the device.
    }
    const bool result = client_.call(
        "input.touch", json::object { { "contact", contact }, { "x", x }, { "y", y }, { "down", down } }, err);
    if (result && !down) {
        inputs_.touches.erase(contact);
    }
    if (!result && down) {
        std::string release_err;
        if (client_.call(
                "input.touch", json::object { { "contact", contact }, { "x", x }, { "y", y }, { "down", false } }, release_err)) {
            inputs_.touches.erase(contact);
        }
    }
    return result;
}

bool IOSSession::release_touch(int contact, std::string& err)
{
    const auto found = inputs_.touches.find(contact);
    if (found == inputs_.touches.end()) {
        return true;
    }
    return touch(contact, found->second.first, found->second.second, false, err);
}

bool IOSSession::send_keyboard(const std::set<uint16_t>& usages, std::string& err)
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    json::array keys;
    for (auto usage : usages) {
        keys.emplace_back(usage);
    }
    return client_.call("input.keyboard", json::object { { "usages", keys } }, err);
}

bool IOSSession::key_down(int key, std::string& err)
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    if (const auto usage = keyboard_usage(key)) {
        inputs_.keyboard.insert(usage);
        if (send_keyboard(inputs_.keyboard, err)) {
            return true;
        }
        std::string release_err;
        if (send_keyboard({ }, release_err)) {
            inputs_.keyboard.clear();
        }
        return false;
    }
    if (const auto usage = consumer_usage(key)) {
        inputs_.buttons.insert(usage);
        if (client_.call("input.button", json::object { { "page", 12 }, { "usage", usage }, { "down", true } }, err)) {
            return true;
        }
        std::string release_err;
        if (client_.call("input.button", json::object { { "page", 12 }, { "usage", usage }, { "down", false } }, release_err)) {
            inputs_.buttons.erase(usage);
        }
        return false;
    }
    err = "unsupported iOS Android keycode " + std::to_string(key);
    return false;
}

bool IOSSession::key_up(int key, std::string& err)
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    if (const auto usage = keyboard_usage(key)) {
        auto next = inputs_.keyboard;
        next.erase(usage);
        if (!send_keyboard(next, err)) {
            return false;
        }
        inputs_.keyboard = std::move(next);
        return true;
    }
    if (const auto usage = consumer_usage(key)) {
        if (!inputs_.buttons.contains(usage)) {
            return true;
        }
        if (!client_.call("input.button", json::object { { "page", 12 }, { "usage", usage }, { "down", false } }, err)) {
            return false;
        }
        inputs_.buttons.erase(usage);
        return true;
    }
    err = "unsupported iOS Android keycode " + std::to_string(key);
    return false;
}

bool IOSSession::type_text(const std::string& text, std::string& err)
{
    if (!client_.connected()) {
        err = "not connected";
        return false;
    }
    if (!ascii_input_supported(text)) {
        err = "iOS input_text requires nonempty US-layout ASCII; unsupported text is not partially sent";
        return false;
    }
    if (!inputs_.keyboard.empty() || inputs_.keyboard_release_pending) {
        err = "release held keys before iOS input_text";
        return false;
    }
    inputs_.keyboard_release_pending = true;
    if (client_.call("input.text", json::object { { "text", text }, { "hold_ms", 40 } }, err)) {
        inputs_.keyboard_release_pending = false;
        return true;
    }
    std::string release_err;
    if (send_keyboard({ }, release_err)) {
        inputs_.keyboard_release_pending = false;
    }
    return false;
}

bool IOSSession::release_inputs(std::string& err)
{
    return inputs_.release(
        [this](int contact, double x, double y, std::string& error) {
            if (!client_.connected()) {
                error = "iOS touch service is unavailable during cleanup";
                return false;
            }
            return client_.call("input.touch", json::object { { "contact", contact }, { "x", x }, { "y", y }, { "down", false } }, error);
        },
        [this](std::string& error) { return send_keyboard({ }, error); },
        [this](uint16_t usage, std::string& error) {
            if (!client_.connected()) {
                error = "iOS hardware button service is unavailable during cleanup";
                return false;
            }
            return client_.call("input.button", json::object { { "page", 12 }, { "usage", usage }, { "down", false } }, error);
        },
        err);
}

std::string IOSSession::device_property(const char* key) const
{
    if (std::string_view(key) == "OSVersion") {
        return properties_.get("os_version", std::string());
    }
    if (std::string_view(key) == "ProductType") {
        return properties_.get("product_type", std::string());
    }
    return { };
}

bool IOSSession::launch_app(const std::string& bundle, std::string& err)
{
    return client_.call("app.launch", json::object { { "bundle_id", bundle } }, err);
}

bool IOSSession::stop_app(const std::string& bundle, std::string& err)
{
    return client_.call("app.stop", json::object { { "bundle_id", bundle } }, err);
}
} // namespace maa::ios_unit
