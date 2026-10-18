#include "IOSSession.h"
#include "wifi/Credentials.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <thread>
#include <utility>

#include <MaaUtils/Logger.h>
#include <MaaUtils/NoWarningCV.hpp>
#include <opencv2/imgproc.hpp>

#include "hid/Hid.h"
#include "media/FramePump.h"
#include "remote/App.h"
#include "remote/Device.h"
#include "remote/DeviceConnection.h"
#include "remote/DisplayInfo.h"
#include "transport/IoBudget.h"
#include "xpc/XpcValue.h"

namespace maa::ios_unit
{
namespace
{
constexpr int kFrameWaitMs = 3000;
constexpr int kFirstFrameWaitMs = 300;
constexpr uint64_t kDisplayId = 1;

uint64_t pipeline_progress(const maa::ios::media::FramePump::Stats& stats)
{
    return stats.aus + stats.decoded + stats.no_output + stats.dropped + stats.dropped_oversized + stats.dropped_awaiting_keyframe
           + stats.dropped_fragments + stats.gaps + stats.restarts;
}

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
} // namespace

IOSSession::IOSSession() = default;

IOSSession::~IOSSession()
{
    close();
}

bool IOSSession::create(
    const std::string& udid,
    const std::string& wifi_address,
    MaaIOScreencapMethod methods,
    std::string& err,
    const std::string& config)
{
    close();
    constexpr auto known = MaaIOScreencapMethod_Stream | MaaIOScreencapMethod_ScreenshotService;
    if ((methods & known) == 0) {
        err = "no supported iOS screencap method selected";
        return false;
    }
    const auto parsed = nlohmann::json::parse(config, nullptr, false);
    std::vector<maa::ios::wifi::PairRecord> records;
    if (!parsed.is_object() || !maa::ios::wifi::parse_credentials(parsed, records, err)) {
        if (err.empty()) {
            err = "Invalid iOS controller configuration";
        }
        return false;
    }
    screencap_methods_ = methods & known;
    maa::ios::transport::IoBudget budget;
    auto device = maa::ios::remote::open_device(
        udid,
        wifi_address,
        err,
        maa::ios::wifi::kAdvertisedPortFallback,
        [&budget] { return budget.stopped(); },
        records);
    if (!device) {
        return false;
    }
    device_ = std::make_unique<maa::ios::remote::Device>(std::move(*device));
    udid_ = device_->udid();
    bool ready = false;
    {
        maa::ios::transport::BudgetWatchdog watchdog(budget, [this] { device_->interrupt_startup(); });
        ready = prepare_services(err);
        if (!watchdog.finish(err)) {
            ready = false;
        }
    }
    if (!ready) {
        close();
        return false;
    }
    return true;
}

bool IOSSession::prepare_services(std::string& err)
{
    std::string watcher_err;
    watcher_ = maa::ios::remote::DisplayWatcher::start(*device_, kDisplayId, watcher_err);
    if (!watcher_) {
        LogWarn << "iOS display subscription unavailable" << VAR(watcher_err);
    }

    if ((screencap_methods_ & MaaIOScreencapMethod_Stream) != 0) {
        maa::ios::media::FramePump::Options options;
        options.silence_restart_ms = 0;
        options.frame_geometry = [this]() -> std::optional<maa::ios::FrameGeometry> {
            if (!watcher_) {
                return std::nullopt;
            }
            const auto state = watcher_->latest();
            return maa::ios::FrameGeometry { state.width, state.height, state.orientation };
        };
        pump_ = maa::ios::media::FramePump::start(*device_, options, err);
        if (!pump_ && (screencap_methods_ & MaaIOScreencapMethod_ScreenshotService) == 0) {
            return false;
        }
        if (!pump_) {
            LogWarn << "iOS video unavailable, using screenshot service" << VAR(err);
            err.clear();
        }
    }
    hid_ = maa::ios::hid::Service::open(*device_, err);
    return hid_ != nullptr;
}

void IOSSession::close()
{
    std::string err;
    if (!release_inputs(err)) {
        LogWarn << "iOS input cleanup failed" << VAR(err);
    }
    hid_.reset();
    buttons_.reset();
    pump_.reset();
    watcher_.reset();
    device_.reset();
    udid_.clear();
    mapping_ = { };
    inputs_.keyboard.clear();
    inputs_.keyboard_release_pending = false;
    inputs_.buttons.clear();
    inputs_.touch_active = false;
}

bool IOSSession::convert(const maa::ios::Frame& frame, cv::Mat& image, std::string& err)
{
    if (!frame || std::cmp_greater(frame.width, std::numeric_limits<int>::max())
        || std::cmp_greater(frame.height, std::numeric_limits<int>::max()) || frame.bytes_per_pixel != 4
        || frame.row_pitch < uint64_t(frame.width) * 4 || frame.pixels.size() < uint64_t(frame.row_pitch) * frame.height) {
        err = "invalid decoded iOS video frame";
        return false;
    }
    const int coded_w = static_cast<int>(frame.width);
    const int coded_h = static_cast<int>(frame.height);
    auto crop = maa::ios::media::display_crop(coded_w, coded_h);
    ImageMapping mapping { crop.w, crop.h, 0, true };
    if (frame.geometry) {
        const auto& geometry = *frame.geometry;
        if (geometry.display_width > 0 && geometry.display_height > 0) {
            crop = { 0, 0, geometry.display_width, geometry.display_height };
            mapping.width = crop.w;
            mapping.height = crop.h;
        }
        mapping.degrees = orientation_degrees(geometry.orientation).value_or(0);
    }
    if (crop.x < 0 || crop.y < 0 || crop.w <= 0 || crop.h <= 0 || int64_t(crop.x) + crop.w > coded_w
        || int64_t(crop.y) + crop.h > coded_h) {
        err = "iOS frame dimensions do not match its display geometry";
        return false;
    }
    cv::Mat bgra(coded_h, coded_w, CV_8UC4, const_cast<uint8_t*>(frame.pixels.data()), frame.row_pitch);
    cv::cvtColor(bgra(cv::Rect(crop.x, crop.y, crop.w, crop.h)), image, cv::COLOR_BGRA2BGR);
    rotate_image(image, mapping.degrees);
    if (image.empty()) {
        err = "empty iOS video image";
        return false;
    }
    mapping_ = mapping;
    return true;
}

bool IOSSession::logical_to_panel(int x, int y, int width, int height, double& fx, double& fy, std::string& err) const
{
    if (!device_) {
        err = "not connected";
        return false;
    }
    if (!mapping_.to_panel(x, y, width, height, fx, fy)) {
        err = "no valid geometry for the delivered screenshot; capture again before input";
        return false;
    }
    return true;
}

bool IOSSession::screencap(cv::Mat& image, std::string& err)
{
    if (!device_) {
        err = "not connected";
        return false;
    }
    const bool allow_service = (screencap_methods_ & MaaIOScreencapMethod_ScreenshotService) != 0;
    if (pump_ && !pump_->video_unusable()) {
        const auto floor = pump_->serial();
        const auto before = pump_->stats();
        pump_->wake();
        maa::ios::Frame frame;
        auto serial = pump_->newer(frame, floor, kFirstFrameWaitMs);
        if (serial == 0 && pump_->reviving()) {
            serial = pump_->newer(frame, floor, kFrameWaitMs);
        }
        if (serial != 0 && convert(frame, image, err)) {
            return true;
        }
        const bool progressing = pipeline_progress(pump_->stats()) > pipeline_progress(before);
        // A quiet, healthy stream may legitimately reuse its last frame. Its geometry travels with it.
        if (!progressing && !pump_->reviving() && pump_->serial() != 0 && pump_->latest(frame, 0) && convert(frame, image, err)) {
            return true;
        }
    }
    if (!allow_service) {
        if (err.empty()) {
            err = "iOS video produced no usable frame; screenshot service was not selected";
        }
        return false;
    }
    return screencap_via_service(image, err);
}

bool IOSSession::screencap_via_service(cv::Mat& image, std::string& err)
{
    const auto before = watcher_ ? watcher_->latest() : maa::ios::remote::DisplayWatcher::State { };
    auto input = maa::ios::xpc::make_dict();
    maa::ios::xpc::dict_set(input, "displayUniqueID", maa::ios::xpc::make_null());
    maa::ios::xpc::dict_set(input, "requestedFormat", maa::ios::xpc::make_string("png"));
    maa::ios::xpc::Value output;
    if (!device_->feature(
            "com.apple.coredevice.screencaptureservice",
            "com.apple.coredevice.feature.capturescreenshot",
            "com.apple.coredevice.action.capturescreenshot",
            input,
            output,
            err,
            false,
            10000)) {
        return false;
    }
    const auto after = watcher_ ? watcher_->latest() : maa::ios::remote::DisplayWatcher::State { };
    if (before.seq != after.seq) {
        err = "iOS geometry changed during screenshot; retry capture";
        return false;
    }
    const auto* png = output.find("image");
    if (!png || png->data.empty() || png->data.size() > std::numeric_limits<int>::max()) {
        err = "iOS screenshot reply has no usable PNG";
        return false;
    }
    auto decoded = cv::imdecode({ png->data.data(), static_cast<int>(png->data.size()) }, cv::IMREAD_COLOR);
    if (decoded.empty()) {
        err = "cannot decode iOS screenshot PNG";
        return false;
    }
    const auto degrees = orientation_degrees(after.orientation);
    const bool swapped = degrees && (*degrees == 90 || *degrees == 270);
    ImageMapping mapping { after.width, after.height, degrees.value_or(0), degrees.has_value() };
    if (mapping.width <= 0 || mapping.height <= 0) {
        mapping.width = swapped ? decoded.rows : decoded.cols;
        mapping.height = swapped ? decoded.cols : decoded.rows;
    }
    mapping.valid = mapping.valid && decoded.cols == (swapped ? mapping.height : mapping.width)
                    && decoded.rows == (swapped ? mapping.width : mapping.height);
    mapping_ = mapping;
    image = std::move(decoded);
    err.clear();
    return true;
}

bool IOSSession::touch(double x, double y, bool down, std::string& err)
{
    if (!hid_) {
        err = "not connected";
        return false;
    }
    inputs_.last_touch = { x, y };
    if (down) {
        inputs_.touch_active = true; // Failed sends may already have reached the device.
    }
    const bool result = hid_->touch(maa::ios::hid::kSurfaceMainTouchscreen, x, y, down, err);
    if (result && !down) {
        inputs_.touch_active = false;
    }
    if (!result && down) {
        std::string release_err;
        if (hid_->touch(maa::ios::hid::kSurfaceMainTouchscreen, x, y, false, release_err)) {
            inputs_.touch_active = false;
        }
    }
    return result;
}

bool IOSSession::release_touch(std::string& err)
{
    return touch(inputs_.last_touch.first, inputs_.last_touch.second, false, err);
}

bool IOSSession::tap(double x, double y, int hold_ms, std::string& err)
{
    if (!touch(x, y, true, err)) {
        return false;
    }
    if (hold_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    }
    return touch(x, y, false, err);
}

bool IOSSession::stroke(const std::vector<std::pair<double, double>>& points, int step_ms, std::string& err)
{
    if (points.size() < 2) {
        err = "iOS swipe needs at least two points";
        return false;
    }
    for (size_t i = 0; i < points.size(); ++i) {
        if (!touch(points[i].first, points[i].second, i + 1 != points.size(), err)) {
            return false;
        }
        if (step_ms > 0 && i + 1 != points.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
        }
    }
    return true;
}

bool IOSSession::send_keyboard(const std::set<uint16_t>& usages, std::string& err)
{
    if (!hid_) {
        err = "not connected";
        return false;
    }
    return hid_->send_report(maa::ios::hid::kSurfaceKeyboard, maa::ios::hid::keyboard_report({ usages.begin(), usages.end() }), err);
}

bool IOSSession::ensure_buttons(std::string& err)
{
    if (!device_) {
        err = "not connected";
        return false;
    }
    if (!buttons_) {
        buttons_ = maa::ios::hid::Buttons::open(*device_, err);
    }
    return buttons_ != nullptr;
}

bool IOSSession::key_down(int key, std::string& err)
{
    if (!hid_) {
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
        if (!ensure_buttons(err)) {
            return false;
        }
        inputs_.buttons.insert(usage);
        if (buttons_->down(maa::ios::hid::button::kUsagePageConsumer, usage, err)) {
            return true;
        }
        std::string release_err;
        if (buttons_->release(maa::ios::hid::button::kUsagePageConsumer, usage, release_err)) {
            inputs_.buttons.erase(usage);
        }
        return false;
    }
    err = "unsupported iOS Android keycode " + std::to_string(key);
    return false;
}

bool IOSSession::key_up(int key, std::string& err)
{
    if (!hid_) {
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
        if (!ensure_buttons(err) || !buttons_->release(maa::ios::hid::button::kUsagePageConsumer, usage, err)) {
            return false;
        }
        inputs_.buttons.erase(usage);
        return true;
    }
    err = "unsupported iOS Android keycode " + std::to_string(key);
    return false;
}

bool IOSSession::click_key(int key, std::string& err)
{
    const auto usage = keyboard_usage(key);
    const auto consumer = consumer_usage(key);
    if ((usage && inputs_.keyboard.contains(usage)) || (consumer && inputs_.buttons.contains(consumer))) {
        err = "iOS key is already held; release it before click_key";
        return false;
    }
    if (!key_down(key, err)) {
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(90));
    return key_up(key, err);
}

bool IOSSession::type_text(const std::string& text, std::string& err)
{
    if (!hid_) {
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
    if (hid_->type_text(text, 40, err)) {
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
        [this](double x, double y, std::string& error) {
            if (!hid_) {
                error = "iOS touch service is unavailable during cleanup";
                return false;
            }
            return hid_->touch(maa::ios::hid::kSurfaceMainTouchscreen, x, y, false, error);
        },
        [this](std::string& error) { return send_keyboard({ }, error); },
        [this](uint16_t usage, std::string& error) {
            if (!buttons_) {
                error = "iOS hardware button service is unavailable during cleanup";
                return false;
            }
            return buttons_->release(maa::ios::hid::button::kUsagePageConsumer, usage, error);
        },
        err);
}

std::string IOSSession::device_property(const char* key) const
{
    return device_ ? device_->property(key) : std::string { };
}

bool IOSSession::launch_app(const std::string& bundle_id, std::string& err)
{
    if (!device_) {
        err = "not connected";
        return false;
    }
    return maa::ios::remote::App::launch(*device_, bundle_id, err, false);
}

bool IOSSession::stop_app(const std::string& bundle_id, std::string& err)
{
    if (!device_) {
        err = "not connected";
        return false;
    }
    return maa::ios::remote::App::stop(*device_, bundle_id, err);
}

} // namespace maa::ios_unit
