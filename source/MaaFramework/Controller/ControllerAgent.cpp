#include "ControllerAgent.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <limits>
#include <string_view>
#include <thread>
#include <vector>

#include "Global/OptionMgr.h"
#include "Global/PluginMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Time.hpp"
#include "Resource/ResourceMgr.h"

namespace
{
constexpr float kLinearToSrgbThreshold = 0.0031308f;
constexpr float kInvSrgbGamma = 1.0f / 2.4f;
constexpr float kHdrDetectEpsilon = 1e-3f;
constexpr float kMinLuminance = 1e-6f;
constexpr float kHdrMiddleGray = 0.18f;
constexpr float kHdrTargetHighlight = 0.85f;
constexpr float kHdrMinExposure = 1.0f / 32.0f;
constexpr float kHdrMaxExposure = 4.0f;
constexpr size_t kHdrTargetSamples = 16384;
constexpr int kHdrProbeImageCount = 5;
constexpr std::string_view kHdrScreenshotEnabledMsg = "Controller.Screenshot.HdrCapture.Enabled";
constexpr std::string_view kHdrDisplayCompensationEnabledMsg = "Controller.Screenshot.HdrDisplayCompensation.Enabled";
constexpr std::string_view kHdrScreenshotDisabledMsg = "Controller.Screenshot.HdrCapture.Disabled";

struct HdrSceneStats
{
    float min_linear = 0.0f;
    float max_linear = 0.0f;
    float avg_log_luminance = 0.0f;
    float highlight_luminance = 0.0f;
    bool valid = false;
};

bool is_hdr_rgba_image(const cv::Mat& image)
{
    return image.type() == CV_16FC4 || image.type() == CV_32FC4;
}

bool get_json_bool(const json::object& obj, const std::string& key)
{
    auto opt = obj.find(key);
    return opt.has_value() && opt->is_boolean() && opt->as_boolean();
}

float tone_map_filmic(float linear)
{
    linear = std::max(linear, 0.0f);
    const float numerator = linear * (2.51f * linear + 0.03f);
    const float denominator = linear * (2.43f * linear + 0.59f) + 0.14f;
    if (denominator <= 0.0f) {
        return 0.0f;
    }
    return std::clamp(numerator / denominator, 0.0f, 1.0f);
}

uint8_t linear_to_srgb_u8(float linear)
{
    linear = std::clamp(linear, 0.0f, 1.0f);

    const float srgb = linear <= kLinearToSrgbThreshold ? linear * 12.92f : 1.055f * std::pow(linear, kInvSrgbGamma) - 0.055f;
    return static_cast<uint8_t>(std::lround(std::clamp(srgb, 0.0f, 1.0f) * 255.0f));
}

cv::Mat hdr_rgba_to_bgr8(
    const cv::Mat& hdr_rgba,
    bool* tone_mapping_applied = nullptr) // 转换HDR RGBA图像到8位BGR图像，并自动应用电影式色调映射
{
    if (hdr_rgba.empty()) {
        if (tone_mapping_applied) {
            *tone_mapping_applied = false;
        }
        return { };
    }

    cv::Mat rgba32f;
    if (hdr_rgba.type() == CV_32FC4) {
        rgba32f = hdr_rgba;
    }
    else {
        hdr_rgba.convertTo(rgba32f, CV_32FC4);
    }

    const auto analyze_scene = [&]() -> HdrSceneStats {
        HdrSceneStats stats;

        const double total_pixels = static_cast<double>(rgba32f.rows) * rgba32f.cols;
        const int stride = std::max(1, static_cast<int>(std::sqrt(total_pixels / static_cast<double>(kHdrTargetSamples))));

        stats.min_linear = std::numeric_limits<float>::max();
        stats.max_linear = std::numeric_limits<float>::lowest();

        std::vector<float> luminances;
        luminances.reserve(static_cast<size_t>((rgba32f.rows / stride + 1) * (rgba32f.cols / stride + 1)));

        double log_sum = 0.0;
        size_t sample_count = 0;

        for (int y = 0; y < rgba32f.rows; y += stride) {
            const auto* row = rgba32f.ptr<cv::Vec4f>(y);
            for (int x = 0; x < rgba32f.cols; x += stride) {
                const cv::Vec4f& pixel = row[x];
                for (int c = 0; c < 3; ++c) {
                    stats.min_linear = std::min(stats.min_linear, pixel[c]);
                    stats.max_linear = std::max(stats.max_linear, pixel[c]);
                }

                const float r = std::max(pixel[0], 0.0f);
                const float g = std::max(pixel[1], 0.0f);
                const float b = std::max(pixel[2], 0.0f);
                const float luminance = std::max(kMinLuminance, 0.2126f * r + 0.7152f * g + 0.0722f * b);

                luminances.emplace_back(luminance);
                log_sum += std::log(luminance);
                ++sample_count;
            }
        }

        if (sample_count == 0 || luminances.empty()) {
            return stats;
        }

        const size_t highlight_index = static_cast<size_t>(std::floor((luminances.size() - 1) * 0.95));
        std::nth_element(luminances.begin(), luminances.begin() + highlight_index, luminances.end());

        stats.avg_log_luminance = static_cast<float>(std::exp(log_sum / static_cast<double>(sample_count)));
        stats.highlight_luminance = luminances[highlight_index];
        stats.valid = true;
        return stats;
    };

    const HdrSceneStats stats = analyze_scene();
    const bool needs_tone_mapping = stats.valid && (stats.max_linear > 1.0f + kHdrDetectEpsilon || stats.min_linear < -kHdrDetectEpsilon);
    if (tone_mapping_applied) {
        *tone_mapping_applied = needs_tone_mapping;
    }
    float exposure = 1.0f;
    if (needs_tone_mapping) {
        if (stats.avg_log_luminance > kMinLuminance) {
            exposure = kHdrMiddleGray / stats.avg_log_luminance;
        }
        if (stats.highlight_luminance > kMinLuminance) {
            exposure = std::min(exposure, kHdrTargetHighlight / stats.highlight_luminance);
        }
        exposure = std::clamp(exposure, kHdrMinExposure, kHdrMaxExposure);

        static auto last_hdr_log_time = std::chrono::steady_clock::time_point { };
        const auto now = std::chrono::steady_clock::now();
        if (last_hdr_log_time == std::chrono::steady_clock::time_point { } || now - last_hdr_log_time > std::chrono::seconds(5)) {
            last_hdr_log_time = now;
            LogInfo << "HDR screenshot tone mapping" << VAR(stats.min_linear) << VAR(stats.max_linear) << VAR(stats.avg_log_luminance)
                    << VAR(stats.highlight_luminance) << VAR(exposure);
        }
    }

    cv::Mat bgr8(rgba32f.rows, rgba32f.cols, CV_8UC3);

    cv::parallel_for_(cv::Range(0, rgba32f.rows), [&](const cv::Range& range) {
        for (int y = range.start; y < range.end; ++y) {
            const auto* src_row = rgba32f.ptr<cv::Vec4f>(y);
            auto* dst_row = bgr8.ptr<cv::Vec3b>(y);

            for (int x = 0; x < rgba32f.cols; ++x) {
                float r = std::max(src_row[x][0], 0.0f);
                float g = std::max(src_row[x][1], 0.0f);
                float b = std::max(src_row[x][2], 0.0f);

                if (needs_tone_mapping) {
                    r *= exposure;
                    g *= exposure;
                    b *= exposure;

                    const float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                    const float mapped_luminance = tone_map_filmic(luminance);
                    const float scale = luminance > kMinLuminance ? (mapped_luminance / luminance) : 0.0f;

                    r *= scale;
                    g *= scale;
                    b *= scale;
                }

                dst_row[x] = cv::Vec3b(linear_to_srgb_u8(b), linear_to_srgb_u8(g), linear_to_srgb_u8(r));
            }
        }
    });

    return bgr8;
}
} // namespace

MAA_CTRL_NS_BEGIN

ControllerAgent::ControllerAgent(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit)
    : control_unit_(std::move(control_unit))
{
    LogFunc << VAR(control_unit_);

    auto& plugin_mgr = MAA_GLOBAL_NS::PluginMgr::get_instance();
    LogInfo << VAR(plugin_mgr.get_names());

    for (const auto& sink : plugin_mgr.get_ctrl_sinks()) {
        add_sink(sink, this);
    }

    action_runner_ =
        std::make_unique<AsyncRunner<Action>>(std::bind(&ControllerAgent::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerAgent::~ControllerAgent()
{
    LogFunc;
}

bool ControllerAgent::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogInfo << VAR(key) << VAR(value) << VAR(val_size);

    switch (key) {
    case MaaCtrlOption_ScreenshotTargetLongSide:
        return set_image_target_long_side(value, val_size);
    case MaaCtrlOption_ScreenshotTargetShortSide:
        return set_image_target_short_side(value, val_size);
    case MaaCtrlOption_ScreenshotUseRawSize:
        return set_image_use_raw_size(value, val_size);
    case MaaCtrlOption_MouseLockFollow:
        return set_mouse_lock_follow_option(value, val_size);
    case MaaCtrlOption_ScreenshotResizeMethod:
        return set_screenshot_resize_method(value, val_size);
    case MaaCtrlOption_BackgroundManagedKeys:
        return set_background_managed_keys_option(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaCtrlId ControllerAgent::post_connection()
{
    return post({ .type = Action::Type::connect }, true);
}

MaaCtrlId ControllerAgent::post_click(int x, int y, int contact, int pressure)
{
    ClickParam p { .point = { x, y }, .contact = contact, .pressure = pressure };
    return post({ .type = Action::Type::click, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_swipe(int x1, int y1, int x2, int y2, int duration, int contact, int pressure)
{
    const uint safe_duration = static_cast<uint>(std::max(duration, 0));
    SwipeParam p { .begin = { x1, y1 },
                   .end = { { x2, y2 } },
                   .duration = { safe_duration },
                   .contact = contact,
                   .pressure = pressure };
    return post({ .type = Action::Type::swipe, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_click_key(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    return post({ .type = Action::Type::click_key, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_input_text(const std::string& text)
{
    InputTextParam p { .text = text };
    return post({ .type = Action::Type::input_text, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_start_app(const std::string& intent)
{
    AppParam p { .package = intent };
    return post({ .type = Action::Type::start_app, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_stop_app(const std::string& intent)
{
    AppParam p { .package = intent };
    return post({ .type = Action::Type::stop_app, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_screencap()
{
    return post({ .type = Action::Type::screencap }, true);
}

MaaCtrlId ControllerAgent::post_touch_down(int contact, int x, int y, int pressure)
{
    TouchParam p { .contact = contact, .point = { x, y }, .pressure = pressure };
    return post({ .type = Action::Type::touch_down, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_touch_move(int contact, int x, int y, int pressure)
{
    TouchParam p { .contact = contact, .point = { x, y }, .pressure = pressure };
    return post({ .type = Action::Type::touch_move, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_touch_up(int contact)
{
    TouchParam p { .contact = contact };
    return post({ .type = Action::Type::touch_up, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_relative_move(int dx, int dy)
{
    RelativeMoveParam p { .dx = dx, .dy = dy };
    return post({ .type = Action::Type::relative_move, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_key_down(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    return post({ .type = Action::Type::key_down, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_key_up(int keycode)
{
    ClickKeyParam p { .keycode = { keycode } };
    return post({ .type = Action::Type::key_up, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_scroll(int dx, int dy)
{
    ScrollParam p { .dx = dx, .dy = dy };
    return post({ .type = Action::Type::scroll, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_shell(const std::string& cmd, int64_t timeout)
{
    ShellParam p { .cmd = cmd, .shell_timeout = timeout };
    return post({ .type = Action::Type::shell, .param = std::move(p) }, true);
}

MaaCtrlId ControllerAgent::post_inactive()
{
    return post({ .type = Action::Type::inactive }, true);
}

MaaStatus ControllerAgent::status(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    return static_cast<MaaStatus>(action_runner_->status(ctrl_id));
}

MaaStatus ControllerAgent::wait(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    if (ctrl_id == MaaInvalidId) {
        return MaaStatus_Invalid;
    }

    action_runner_->wait(ctrl_id);
    return status(ctrl_id);
}

bool ControllerAgent::connected() const
{
    if (!control_unit_) {
        return false;
    }
    return control_unit_->connected();
}

cv::Mat ControllerAgent::cached_image() const
{
    std::unique_lock lock(image_mutex_);
    return image_.clone();
}

std::string ControllerAgent::cached_shell_output() const
{
    std::unique_lock lock(shell_output_mutex_);
    return shell_output_;
}

std::string ControllerAgent::get_uuid()
{
    {
        std::unique_lock lock(uuid_mutex_);
        if (!uuid_cache_.empty()) {
            return uuid_cache_;
        }
    }

    request_uuid();

    std::unique_lock lock(uuid_mutex_);
    return uuid_cache_;
}

bool ControllerAgent::get_resolution(int32_t& width, int32_t& height) const
{
    std::unique_lock lock(image_mutex_);

    if (image_raw_width_ == 0 || image_raw_height_ == 0) {
        return false;
    }

    width = image_raw_width_;
    height = image_raw_height_;
    return true;
}

json::object ControllerAgent::get_info() const
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return {};
    }
    return control_unit_->get_info();
}

MaaSinkId ControllerAgent::add_sink(MaaEventCallback callback, void* trans_arg)
{
    return notifier_.add_sink(callback, trans_arg);
}

void ControllerAgent::remove_sink(MaaSinkId sink_id)
{
    notifier_.remove_sink(sink_id);
}

void ControllerAgent::clear_sinks()
{
    notifier_.clear_sinks();
}

void ControllerAgent::post_stop()
{
    LogFunc;

    {
        std::unique_lock lock(stop_mutex_);
        need_to_stop_ = true;
    }

    if (action_runner_ && action_runner_->running()) {
        action_runner_->clear();
    }
}

bool ControllerAgent::running() const
{
    return action_runner_ && action_runner_->running();
}

bool ControllerAgent::click(ClickParam p)
{
    auto id = post({ .type = Action::Type::click, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::long_press(LongPressParam p)
{
    auto id = post({ .type = Action::Type::long_press, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::swipe(SwipeParam p)
{
    auto id = post({ .type = Action::Type::swipe, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::multi_swipe(MultiSwipeParam p)
{
    auto id = post({ .type = Action::Type::multi_swipe, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_down(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_down, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_move(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_move, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::touch_up(TouchParam p)
{
    auto id = post({ .type = Action::Type::touch_up, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::relative_move(RelativeMoveParam p)
{
    auto id = post({ .type = Action::Type::relative_move, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::click_key(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::click_key, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::long_press_key(LongPressKeyParam p)
{
    auto id = post({ .type = Action::Type::long_press_key, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::key_down(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::key_down, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::key_up(ClickKeyParam p)
{
    auto id = post({ .type = Action::Type::key_up, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::input_text(InputTextParam p)
{
    auto id = post({ .type = Action::Type::input_text, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

cv::Mat ControllerAgent::screencap()
{
    auto id = post({ .type = Action::Type::screencap });
    if (wait(id) != MaaStatus_Succeeded) {
        return { };
    }
    return cached_image();
}

bool ControllerAgent::start_app(AppParam p)
{
    auto id = post({ .type = Action::Type::start_app, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::stop_app(AppParam p)
{
    auto id = post({ .type = Action::Type::stop_app, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::scroll(ScrollParam p)
{
    auto id = post({ .type = Action::Type::scroll, .param = std::move(p) });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::shell(const std::string& cmd, std::string& output, int64_t timeout)
{
    ShellParam p { .cmd = cmd, .shell_timeout = timeout };
    auto id = post({ .type = Action::Type::shell, .param = std::move(p) });
    bool ret = wait(id) == MaaStatus_Succeeded;
    if (ret) {
        output = cached_shell_output();
    }
    return ret;
}

bool ControllerAgent::handle_inactive()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->inactive();

    return ret;
}

MaaCtrlId ControllerAgent::post(Action action, bool notify)
{
    // LogInfo << VAR(action.type) << VAR(action.param);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    if (!action_runner_) {
        return MaaInvalidId;
    }
    std::unique_lock lock { focus_ids_mutex_ };
    MaaCtrlId id = action_runner_->post(std::move(action));
    if (notify && id != MaaInvalidId) {
        focus_ids_.emplace(id);
    }
    return id;
}

bool ControllerAgent::handle_connect()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->connect();

    request_uuid();
    {
        std::unique_lock lock(image_mutex_);
        hdr_screenshot_mode_notified_ = false;
        last_hdr_capture_active_ = false;
        last_hdr_gpu_processed_ = false;
        last_hdr_display_compensated_ = false;
        hdr_probe_images_remaining_ = 0;
    }

    return ret;
}

bool ControllerAgent::handle_click(const ClickParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);

    bool ret = true;
    if (control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick) {
        ret &= control_unit_->touch_down(param.contact, point.x, point.y, param.pressure);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ret &= control_unit_->touch_up(param.contact);
    }
    else {
        ret &= control_unit_->click(point.x, point.y);
    }

    return ret;
}

bool ControllerAgent::handle_long_press(const LongPressParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);

    bool ret = true;
    if (control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick) {
        ret &= control_unit_->touch_down(param.contact, point.x, point.y, param.pressure);
        std::this_thread::sleep_for(std::chrono::milliseconds(param.duration));
        ret &= control_unit_->touch_up(param.contact);
    }
    else {
        LogWarn << "long press not supported, use click instead";
        ret &= control_unit_->click(point.x, point.y);
    }

    return ret;
}

bool ControllerAgent::handle_swipe(const SwipeParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    const bool use_touch_down_up = control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    if (!use_touch_down_up) {
        LogWarn << "touch not supported, use swipe instead. some features can not work";
    }

    cv::Point begin = preproc_touch_point(param.begin);
    bool ret = !param.end.empty();

    if (!param.only_hover && use_touch_down_up) {
        ret &= control_unit_->touch_down(param.contact, begin.x, begin.y, param.pressure);
    }

    for (size_t i = 0; i < param.end.size(); ++i) {
        const cv::Point& end = preproc_touch_point(param.end.at(i));
        const uint duration = param.duration.empty() ? 200 : (i < param.duration.size()) ? param.duration.at(i) : param.duration.back();
        const uint end_hold = param.end_hold.empty() ? 0 : (i < param.end_hold.size()) ? param.end_hold.at(i) : param.end_hold.back();

        if (use_touch_down_up) {
            constexpr int kIntervalMs = 10;
            const std::chrono::milliseconds delay(kIntervalMs);
            const size_t total_steps = std::max<size_t>(1, static_cast<size_t>(std::ceil(static_cast<double>(duration) / kIntervalMs)));
            const double x_step_len = static_cast<double>(end.x - begin.x) / static_cast<double>(total_steps);
            const double y_step_len = static_cast<double>(end.y - begin.y) / static_cast<double>(total_steps);

            auto now = std::chrono::steady_clock::now();

            for (size_t step = 1; step <= total_steps; ++step) {
                int mx = end.x;
                int my = end.y;
                if (step < total_steps) {
                    mx = static_cast<int>(std::round(begin.x + step * x_step_len));
                    my = static_cast<int>(std::round(begin.y + step * y_step_len));
                }

                std::this_thread::sleep_until(now + delay);

                now = std::chrono::steady_clock::now();
                ret &= control_unit_->touch_move(param.contact, mx, my, param.pressure);
            }
        }
        else {
            ret &= control_unit_->swipe(begin.x, begin.y, end.x, end.y, duration);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(end_hold));

        begin = end;
    }

    if (!param.only_hover && use_touch_down_up) {
        ret &= control_unit_->touch_up(param.contact);
    }

    return ret;
}

bool ControllerAgent::handle_multi_swipe(const MultiSwipeParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    if (!(control_unit_->get_features() & MaaControllerFeature_UseMouseDownAndUpInsteadOfClick)) {
        LogError << "touch is not available";
        return false;
    }

    constexpr int kIntervalMs = 10;
    const std::chrono::milliseconds delay(kIntervalMs);

    struct SegmentOperating
    {
        cv::Point begin { };
        cv::Point end { };
        size_t total_steps = 1;
        double x_step_len = 0.0;
        double y_step_len = 0.0;
        size_t step_index = 0;
        uint end_hold = 0;
    };

    struct ContactOperating
    {
        std::vector<SegmentOperating> seg;
        size_t seg_index = 0;
        uint hold_until = 0;
        bool started = false;
        bool finished = false;
    };

    const auto calc_total_steps = [](uint duration_ms) -> size_t {
        return std::max<size_t>(1, static_cast<size_t>(std::ceil(static_cast<double>(duration_ms) / kIntervalMs)));
    };

    // contact index < end index < op >>
    std::vector<ContactOperating> operating(param.swipes.size());

    for (size_t s_i = 0; s_i < param.swipes.size(); ++s_i) {
        const SwipeParam& s = param.swipes.at(s_i);

        cv::Point begin = preproc_touch_point(s.begin);
        for (size_t e_i = 0; e_i < s.end.size(); ++e_i) {
            const cv::Point& end = preproc_touch_point(s.end.at(e_i));
            const uint duration = s.duration.empty() ? 200 : (e_i < s.duration.size()) ? s.duration.at(e_i) : s.duration.back();
            const uint end_hold = s.end_hold.empty() ? 0 : (e_i < s.end_hold.size()) ? s.end_hold.at(e_i) : s.end_hold.back();

            SegmentOperating o;
            o.begin = begin;
            o.end = end;
            o.total_steps = calc_total_steps(duration);
            o.x_step_len = static_cast<double>(end.x - begin.x) / static_cast<double>(o.total_steps);
            o.y_step_len = static_cast<double>(end.y - begin.y) / static_cast<double>(o.total_steps);
            o.end_hold = end_hold;

            operating.at(s_i).seg.emplace_back(std::move(o));

            begin = end;
        }
    }

    const auto starting = std::chrono::steady_clock::now();
    auto now = starting;

    bool ret = !param.swipes.empty();
    size_t finished_count = 0;
    while (finished_count < param.swipes.size()) {
        uint now_point = static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(now - starting).count());

        for (size_t i = 0; i < param.swipes.size(); ++i) {
            const SwipeParam& s = param.swipes.at(i);
            if (now_point < s.starting) {
                continue;
            }

            int contact = s.contact != 0 ? s.contact : static_cast<int>(i);

            if (i >= operating.size()) {
                LogError << "Invalid contact index" << VAR(i) << VAR(operating.size());
                continue;
            }
            auto& contact_op = operating.at(i);

            if (contact_op.finished) {
                continue;
            }
            if (contact_op.seg.empty()) {
                LogError << "Invalid multi swipe: empty path" << VAR(i);
                contact_op.finished = true;
                ++finished_count;
                ret = false;
                continue;
            }
            if (now_point < contact_op.hold_until) {
                continue;
            }
            if (!contact_op.started) {
                if (!s.only_hover) {
                    const auto& first_seg = contact_op.seg.front();
                    ret &= control_unit_->touch_down(contact, first_seg.begin.x, first_seg.begin.y, s.pressure);
                }
                contact_op.started = true;
            }

            if (contact_op.seg_index >= contact_op.seg.size()) {
                if (!s.only_hover) {
                    ret &= control_unit_->touch_up(contact);
                }
                contact_op.finished = true;
                ++finished_count;
                continue;
            }

            auto& seg_op = contact_op.seg.at(contact_op.seg_index);
            if (seg_op.step_index < seg_op.total_steps) {
                ++seg_op.step_index;

                int mx = seg_op.end.x;
                int my = seg_op.end.y;
                if (seg_op.step_index < seg_op.total_steps) {
                    mx = static_cast<int>(std::round(seg_op.begin.x + seg_op.step_index * seg_op.x_step_len));
                    my = static_cast<int>(std::round(seg_op.begin.y + seg_op.step_index * seg_op.y_step_len));
                }

                ret &= control_unit_->touch_move(contact, mx, my, s.pressure);
                if (seg_op.step_index == seg_op.total_steps) {
                    contact_op.hold_until = now_point + seg_op.end_hold;
                }
            }
            else {
                ++contact_op.seg_index;
                contact_op.hold_until = 0;
                if (contact_op.seg_index >= contact_op.seg.size()) {
                    if (!s.only_hover) {
                        ret &= control_unit_->touch_up(contact);
                    }
                    contact_op.finished = true;
                    ++finished_count;
                }
            }
        }

        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();
    }

    return ret;
}

bool ControllerAgent::handle_touch_down(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);
    bool ret = control_unit_->touch_down(param.contact, point.x, point.y, param.pressure);

    return ret;
}

bool ControllerAgent::handle_touch_move(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);
    bool ret = control_unit_->touch_move(param.contact, point.x, point.y, param.pressure);

    return ret;
}

bool ControllerAgent::handle_touch_up(const TouchParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->touch_up(param.contact);

    return ret;
}

bool ControllerAgent::handle_relative_move(const RelativeMoveParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    if (auto unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::RelativeMovableUnit>(control_unit_)) {
        return unit->relative_move(param.dx, param.dy);
    }

    LogError << "Relative move is not supported for this controller type";
    return false;
}

bool ControllerAgent::handle_click_key(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    bool use_key_down_up = control_unit_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;

    for (const auto& keycode : param.keycode) {
        if (use_key_down_up) {
            ret &= control_unit_->key_down(keycode);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ret &= control_unit_->key_up(keycode);
        }
        else {
            ret &= control_unit_->click_key(keycode);
        }
    }

    return ret;
}

bool ControllerAgent::handle_long_press_key(const LongPressKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    bool use_key_down_up = control_unit_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;

    for (const auto& keycode : param.keycode) {
        if (use_key_down_up) {
            ret &= control_unit_->key_down(keycode);
            std::this_thread::sleep_for(std::chrono::milliseconds(param.duration));
            ret &= control_unit_->key_up(keycode);
        }
        else {
            LogWarn << "long press key not supported, use click instead";
            ret &= control_unit_->click_key(keycode);
        }
    }

    return ret;
}

bool ControllerAgent::handle_input_text(const InputTextParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->input_text(param.text);

    return ret;
}

bool ControllerAgent::handle_screencap()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Mat raw_image;
    bool screencaped = control_unit_->screencap(raw_image);
    if (!screencaped) {
        LogError << "controller screencap failed";
        return false;
    }

    bool ret = postproc_screenshot(raw_image);

    return ret;
}

bool ControllerAgent::handle_start_app(const AppParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->start_app(param.package);

    return ret;
}

bool ControllerAgent::handle_stop_app(const AppParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = control_unit_->stop_app(param.package);

    return ret;
}

bool ControllerAgent::handle_key_down(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    for (const auto& keycode : param.keycode) {
        ret &= control_unit_->key_down(keycode);
    }

    return ret;
}

bool ControllerAgent::handle_key_up(const ClickKeyParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    bool ret = !param.keycode.empty();

    for (const auto& keycode : param.keycode) {
        ret &= control_unit_->key_up(keycode);
    }

    return ret;
}

bool ControllerAgent::handle_scroll(const ScrollParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    cv::Point point = preproc_touch_point(param.point);
    if (!control_unit_->touch_move(0, point.x, point.y, 0)) {
        LogWarn << "Failed to move to scroll position" << VAR(point);
    }

    if (auto unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::ScrollableUnit>(control_unit_)) {
        return unit->scroll(param.dx, param.dy);
    }

    LogError << "Scroll is not supported for this controller type";
    return false;
}

bool ControllerAgent::handle_shell(const ShellParam& param)
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    std::string output;
    auto timeout = param.shell_timeout < 0 ? std::chrono::milliseconds::max() : std::chrono::milliseconds(param.shell_timeout);

    auto unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::ShellableUnit>(control_unit_);
    if (!unit) {
        LogError << "Shell is not supported for this controller type";
        return false;
    }

    bool ret = unit->shell(param.cmd, output, timeout);

    if (ret) {
        std::unique_lock lock(shell_output_mutex_);
        shell_output_ = std::move(output);
    }

    return ret;
}

bool ControllerAgent::check_stop()
{
    std::unique_lock lock(stop_mutex_);

    if (!need_to_stop_) {
        return true;
    }

    if (running()) {
        LogError << "stopping, ignore new post";
        return false;
    }

    need_to_stop_ = false;
    return true;
}

bool ControllerAgent::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    bool ret = false;

    bool notify = false;
    {
        std::unique_lock lock { focus_ids_mutex_ };
        notify = focus_ids_.erase(id) > 0;
    }

    const auto make_cb_detail = [&]() -> json::value {
        json::object info;
        if (control_unit_) {
            info = control_unit_->get_info();
        }

        return {
            { "ctrl_id", id },
            { "uuid", get_uuid() },
            { "action", action.type },
            { "param", action.param },
            { "info", std::move(info) },
        };
    };

    if (notify) {
        notifier_.notify(this, MaaMsg_Controller_Action_Starting, make_cb_detail());
    }

    switch (action.type) {
    case Action::Type::connect:
        ret = handle_connect();
        break;

    case Action::Type::click:
        ret = handle_click(std::get<ClickParam>(action.param));
        break;
    case Action::Type::long_press:
        ret = handle_long_press(std::get<LongPressParam>(action.param));
        break;
    case Action::Type::swipe:
        ret = handle_swipe(std::get<SwipeParam>(action.param));
        break;
    case Action::Type::multi_swipe:
        ret = handle_multi_swipe(std::get<MultiSwipeParam>(action.param));
        break;

    case Action::Type::touch_down:
        ret = handle_touch_down(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_move:
        ret = handle_touch_move(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_up:
        ret = handle_touch_up(std::get<TouchParam>(action.param));
        break;

    case Action::Type::click_key:
        ret = handle_click_key(std::get<ClickKeyParam>(action.param));
        break;
    case Action::Type::long_press_key:
        ret = handle_long_press_key(std::get<LongPressKeyParam>(action.param));
        break;
    case Action::Type::input_text:
        ret = handle_input_text(std::get<InputTextParam>(action.param));
        break;

    case Action::Type::key_down:
        ret = handle_key_down(std::get<ClickKeyParam>(action.param));
        break;

    case Action::Type::key_up:
        ret = handle_key_up(std::get<ClickKeyParam>(action.param));
        break;

    case Action::Type::screencap:
        ret = handle_screencap();
        break;

    case Action::Type::start_app:
        ret = handle_start_app(std::get<AppParam>(action.param));
        break;
    case Action::Type::stop_app:
        ret = handle_stop_app(std::get<AppParam>(action.param));
        break;

    case Action::Type::scroll:
        ret = handle_scroll(std::get<ScrollParam>(action.param));
        break;

    case Action::Type::shell:
        ret = handle_shell(std::get<ShellParam>(action.param));
        break;

    case Action::Type::relative_move:
        ret = handle_relative_move(std::get<RelativeMoveParam>(action.param));
        break;

    case Action::Type::inactive:
        ret = handle_inactive();
        break;

    default:
        LogError << "Unknown action type" << VAR(static_cast<int>(action.type));
        ret = false;
    }

    if (notify) {
        notifier_.notify(this, ret ? MaaMsg_Controller_Action_Succeeded : MaaMsg_Controller_Action_Failed, make_cb_detail());
    }

    return ret;
}

cv::Point ControllerAgent::preproc_touch_point(const cv::Point& p)
{
    if (control_unit_ && control_unit_->get_features() & MaaControllerFeature_NoScalingTouchPoints) {
        return p;
    }

    auto load_scale_info = [this]() {
        std::unique_lock lock(image_mutex_);
        return std::array<int, 4> { image_raw_width_, image_raw_height_, image_target_width_, image_target_height_ };
    };

    auto scale_info = load_scale_info();
    if (scale_info[2] == 0 || scale_info[3] == 0) {
        LogWarn << "Invalid image target size" << VAR(scale_info[2]) << VAR(scale_info[3]);

        if (!init_scale_info()) {
            return { };
        }

        scale_info = load_scale_info();
    }

    const int raw_width = scale_info[0];
    const int raw_height = scale_info[1];
    const int target_width = scale_info[2];
    const int target_height = scale_info[3];
    if (raw_width == 0 || raw_height == 0 || target_width == 0 || target_height == 0) {
        LogError << "Invalid scale info" << VAR(raw_width) << VAR(raw_height) << VAR(target_width) << VAR(target_height);
        return { };
    }

    double scale_width = static_cast<double>(raw_width) / target_width;
    double scale_height = static_cast<double>(raw_height) / target_height;

    int proced_x = static_cast<int>(std::round(p.x * scale_width));
    int proced_y = static_cast<int>(std::round(p.y * scale_height));

    return { proced_x, proced_y };
}

bool ControllerAgent::postproc_screenshot(const cv::Mat& raw)
{
    if (raw.empty()) {
        std::unique_lock lock(image_mutex_);
        image_ = cv::Mat();
        LogError << "Empty screenshot";
        return false;
    }

    const json::object control_info = control_unit_ ? control_unit_->get_info() : json::object {};
    const bool hdr_raw_image = is_hdr_rgba_image(raw);
    const bool hdr_capture_active = hdr_raw_image || get_json_bool(control_info, "hdr_capture_active");
    const bool hdr_preprocessed_upstream = !hdr_raw_image && get_json_bool(control_info, "hdr_preprocessed");
    const bool hdr_gpu_processed = get_json_bool(control_info, "hdr_gpu_processed");
    const bool hdr_display_compensated = get_json_bool(control_info, "display_hdr_compensated");
    int target_width = 0;
    int target_height = 0;
    int resize_method = cv::INTER_AREA;
    bool notify_hdr_mode = false;

    {
        std::unique_lock lock(image_mutex_);
        if (raw.cols != image_raw_width_ || raw.rows != image_raw_height_ || image_target_width_ == 0 || image_target_height_ == 0) {
            LogInfo << "Resolution changed" << VAR(raw.cols) << VAR(raw.rows) << VAR(image_raw_width_) << VAR(image_raw_height_);

            image_raw_width_ = raw.cols;
            image_raw_height_ = raw.rows;

            if (!calc_target_image_size()) {
                image_ = cv::Mat();
                LogError << "Invalid target image size";
                return false;
            }
        }

        target_width = image_target_width_;
        target_height = image_target_height_;
        resize_method = image_resize_method_;

        const bool hdr_mode_changed = !hdr_screenshot_mode_notified_ || last_hdr_capture_active_ != hdr_capture_active
                                      || last_hdr_gpu_processed_ != hdr_gpu_processed
                                      || last_hdr_display_compensated_ != hdr_display_compensated;
        if (hdr_mode_changed) {
            hdr_screenshot_mode_notified_ = true;
            last_hdr_capture_active_ = hdr_capture_active;
            last_hdr_gpu_processed_ = hdr_gpu_processed;
            last_hdr_display_compensated_ = hdr_display_compensated;
            hdr_probe_images_remaining_ = (hdr_capture_active || hdr_display_compensated) ? kHdrProbeImageCount : 0;
            notify_hdr_mode = true;
        }
    }

    if (notify_hdr_mode) {
        notify_hdr_screenshot_mode(hdr_capture_active, hdr_gpu_processed, hdr_display_compensated);
    }

    cv::Mat processed;
    if (hdr_capture_active && !hdr_preprocessed_upstream) {
        // Some OpenCV builds are unstable when resizing CV_16FC4 frames directly.
        // Tone-map to an 8-bit preview first, then resize in SDR space.
        cv::Mat hdr_bgr8 = hdr_rgba_to_bgr8(raw);
        if (hdr_bgr8.empty()) {
            std::unique_lock lock(image_mutex_);
            image_ = cv::Mat();
            return false;
        }

        if (hdr_bgr8.cols == target_width && hdr_bgr8.rows == target_height) {
            processed = std::move(hdr_bgr8);
        }
        else {
            cv::resize(hdr_bgr8, processed, { target_width, target_height }, 0, 0, resize_method);
        }
    }
    else {
        cv::Mat resized;
        if (raw.cols == target_width && raw.rows == target_height) {
            resized = raw;
        }
        else {
            cv::resize(raw, resized, { target_width, target_height }, 0, 0, resize_method);
        }

        if (resized.type() == CV_8UC3) {
            processed = resized.clone();
        }
        else if (resized.type() == CV_8UC4) {
            cv::cvtColor(resized, processed, cv::COLOR_BGRA2BGR);
        }
        else {
            LogError << "Unsupported screenshot type" << VAR(resized.type());
            std::unique_lock lock(image_mutex_);
            image_ = cv::Mat();
            return false;
        }
    }

    int probe_index = 0;
    {
        std::unique_lock lock(image_mutex_);
        image_ = processed;

        if (!processed.empty() && hdr_probe_images_remaining_ > 0) {
            probe_index = kHdrProbeImageCount - hdr_probe_images_remaining_ + 1;
            --hdr_probe_images_remaining_;
        }
    }

    if (probe_index > 0) {
        save_hdr_probe_image(processed, probe_index);
    }

    return !processed.empty();
}

void ControllerAgent::notify_hdr_screenshot_mode(bool hdr_capture_active, bool gpu_processed, bool hdr_display_compensated)
{
    const std::string_view message = hdr_capture_active ? kHdrScreenshotEnabledMsg
                                                        : (hdr_display_compensated ? kHdrDisplayCompensationEnabledMsg
                                                                                   : kHdrScreenshotDisabledMsg);
    const std::string_view content =
        hdr_capture_active
            ? (gpu_processed
                   ? "HDR capture path detected. GPU preprocessing is active before readback, and 5 processed screenshots are being saved under `debug/hdr_probe/`. 检测到 HDR 捕获路径，回传内存前已启用 GPU 预处理，并正在额外保存 5 张处理后的截图到 `debug/hdr_probe/`。"
                   : "HDR capture path detected. Processed preview is enabled, and 5 processed screenshots are being saved under `debug/hdr_probe/`. 检测到 HDR 捕获路径，已启用处理后的预览，并正在额外保存 5 张处理后的截图到 `debug/hdr_probe/`。")
            : (hdr_display_compensated
                   ? "HDR display compensation is active for the current screenshot method, and 5 processed screenshots are being saved under `debug/hdr_probe/`. 当前截图方式已启用 HDR 显示补偿，并正在额外保存 5 张处理后的截图到 `debug/hdr_probe/`。"
                   : "HDR capture path not detected. Using the legacy SDR screenshot path. 未检测到 HDR 捕获路径，正在使用原有的 SDR 截图路径。");

    const json::value cb_detail = json::object {
        { "uuid", get_uuid() },
        { "focus",
          json::object {
              { std::string(message),
                json::object {
                    { "content", std::string(content) },
                    { "display", "log" },
                } },
          } },
    };

    notifier_.notify(this, message, cb_detail);
}

void ControllerAgent::save_hdr_probe_image(const cv::Mat& image, int probe_index)
{
    if (image.empty()) {
        LogWarn << "HDR probe image is empty";
        return;
    }

    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();
    const auto probe_dir = std::filesystem::absolute(option.log_dir() / "hdr_probe");
    std::error_code ec;
    std::filesystem::create_directories(probe_dir, ec);
    if (ec) {
        LogError << "Failed to create HDR probe directory" << VAR(probe_dir) << VAR(ec.message());
        return;
    }

    const auto filename = std::format("{}_hdr_probe_{:0>2}.png", format_now_for_filename(), probe_index);
    const auto filepath = probe_dir / path(filename);

    if (imwrite(filepath, image)) {
        LogInfo << "Saved HDR probe screenshot" << VAR(probe_index) << VAR(filepath);
    }
    else {
        LogError << "Failed to save HDR probe screenshot" << VAR(probe_index) << VAR(filepath);
    }
}

bool ControllerAgent::calc_target_image_size()
{
    if (image_raw_width_ == 0 || image_raw_height_ == 0) {
        LogError << "Invalid image raw size";
        return false;
    }

    if (image_use_raw_size_) {
        LogDebug << "image_use_raw_size_" << VAR(image_raw_width_) << VAR(image_raw_height_);
        image_target_width_ = image_raw_width_;
        image_target_height_ = image_raw_height_;
        return true;
    }

    if (image_target_long_side_ == 0 && image_target_short_side_ == 0) {
        LogError << "Invalid image target size";
        return false;
    }

    LogDebug << "Re-calc image target size:" << VAR(image_target_long_side_) << VAR(image_target_short_side_) << VAR(image_raw_width_)
             << VAR(image_raw_height_);

    double scale = static_cast<double>(image_raw_width_) / image_raw_height_;

    if (image_target_short_side_ != 0) {
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = static_cast<int>(std::round(image_target_short_side_ * scale));
            image_target_height_ = image_target_short_side_;
        }
        else {
            image_target_width_ = image_target_short_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_short_side_ / scale));
        }
    }
    else { // image_target_long_side_ != 0
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = image_target_long_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_long_side_ / scale));
        }
        else {
            image_target_width_ = static_cast<int>(std::round(image_target_long_side_ * scale));
            image_target_height_ = image_target_long_side_;
        }
    }

    LogInfo << VAR(image_target_width_) << VAR(image_target_height_);
    return true;
}

void ControllerAgent::clear_target_image_size()
{
    LogDebug;

    image_target_width_ = 0;
    image_target_height_ = 0;
}

bool ControllerAgent::request_uuid()
{
    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    std::string uuid;
    bool ret = control_unit_->request_uuid(uuid);

    std::unique_lock lock(uuid_mutex_);
    if (!ret) {
        uuid_cache_.clear();
        return false;
    }

    uuid_cache_ = std::move(uuid);
    return true;
}

bool ControllerAgent::init_scale_info()
{
    // 实际是通过 postproc_screenshot 初始化的
    return handle_screencap();
}

bool ControllerAgent::set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (!value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != sizeof(int32_t)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    const auto target_long_side = *reinterpret_cast<const int32_t*>(value);
    if (target_long_side < 0) {
        LogError << "invalid image target long side: " << target_long_side;
        return false;
    }

    std::unique_lock lock(image_mutex_);
    image_target_long_side_ = target_long_side;
    image_target_short_side_ = 0;
    clear_target_image_size();

    LogInfo << "image_target_width_ = " << image_target_long_side_;
    return true;
}

bool ControllerAgent::set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (!value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != sizeof(int32_t)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    const auto target_short_side = *reinterpret_cast<const int32_t*>(value);
    if (target_short_side < 0) {
        LogError << "invalid image target short side: " << target_short_side;
        return false;
    }

    std::unique_lock lock(image_mutex_);
    image_target_long_side_ = 0;
    image_target_short_side_ = target_short_side;
    clear_target_image_size();

    LogInfo << "image_target_height_ = " << image_target_short_side_;
    return true;
}

bool ControllerAgent::set_image_use_raw_size(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (!value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != sizeof(bool)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    std::unique_lock lock(image_mutex_);
    image_use_raw_size_ = *reinterpret_cast<const bool*>(value);

    clear_target_image_size();

    return true;
}

bool ControllerAgent::set_mouse_lock_follow_option(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (!value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != sizeof(bool)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }

    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    auto win32_unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI>(control_unit_);
    if (!win32_unit) {
        LogError << "Mouse lock follow is only supported for Win32 controllers.";
        return false;
    }

    bool enabled = *reinterpret_cast<const bool*>(value);
    return win32_unit->set_mouse_lock_follow(enabled);
}

bool ControllerAgent::set_screenshot_resize_method(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (!value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != sizeof(int32_t)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }

    auto raw = *reinterpret_cast<const int32_t*>(value);
    // valid range: cv::INTER_NEAREST(0) ~ cv::INTER_LANCZOS4(4)
    if (raw < cv::INTER_NEAREST || raw > cv::INTER_LANCZOS4) {
        LogError << "invalid resize method: " << raw;
        return false;
    }

    std::unique_lock lock(image_mutex_);
    image_resize_method_ = raw;
    LogInfo << "image_resize_method_ = " << image_resize_method_;
    return true;
}

bool ControllerAgent::set_background_managed_keys_option(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size > 0 && !value) {
        LogError << "option value is nullptr";
        return false;
    }
    if (val_size != 0 && val_size % sizeof(int32_t) != 0) {
        LogError << "invalid value size: " << val_size;
        return false;
    }

    if (!control_unit_) {
        LogError << "control_unit_ is nullptr";
        return false;
    }

    auto win32_unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI>(control_unit_);
    if (!win32_unit) {
        LogError << "Background managed keys is only supported for Win32 controllers.";
        return false;
    }

    size_t count = val_size / sizeof(int32_t);
    auto keycodes = reinterpret_cast<const int32_t*>(value);

    return win32_unit->set_background_managed_keys_option(keycodes, count);
}

MAA_CTRL_NS_END
