#include "ProxyController.h"

#include <format>

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

using namespace MAA_CTRL_UNIT_NS;

MAA_CTRL_NS_BEGIN

ProxyController::ProxyController(MaaController* inner, std::filesystem::path dump_dir)
    : inner_(inner)
    , dump_dir_(std::move(dump_dir))
    , recording_start_(std::chrono::steady_clock::now())
{
    LogFunc << VAR_VOIDP(inner_) << VAR(dump_dir_);

    std::error_code ec;
    std::filesystem::create_directories(dump_dir_, ec);
    if (ec) {
        LogError << "Failed to create dump directory:" << dump_dir_ << VAR(ec.message());
    }

    auto recording_path = dump_dir_ / "recording.jsonl";
    record_file_.open(recording_path, std::ios::out | std::ios::trunc);
    if (!record_file_.is_open()) {
        LogError << "Failed to open recording file:" << recording_path;
    }

    inner_sink_id_ = inner_->add_sink(&ProxyController::inner_sink_callback, this);
}

ProxyController::~ProxyController()
{
    LogFunc;

    if (inner_ && inner_sink_id_ != MaaInvalidId) {
        inner_->remove_sink(inner_sink_id_);
    }

    if (record_file_.is_open()) {
        record_file_.flush();
        record_file_.close();
    }
}

bool ProxyController::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    return inner_->set_option(key, value, val_size);
}

MaaCtrlId ProxyController::post_connection()
{
    auto start = std::chrono::steady_clock::now();
    auto id = inner_->post_connection();
    auto wait_status = inner_->wait(id);
    auto end = std::chrono::steady_clock::now();

    bool success = (wait_status == MaaStatus_Succeeded);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordConnect param;
    param.version = MAA_VERSION;
    if (success) {
        param.uuid = inner_->get_uuid();
        int32_t w = 0, h = 0;
        if (inner_->get_resolution(w, h)) {
            param.width = w;
            param.height = h;
        }
    }

    write_record(make_record_json(make_line(RecordType::connect, success, timestamp, static_cast<int>(cost)), param));
    return id;
}

MaaCtrlId ProxyController::post_click(int x, int y, int contact, int pressure)
{
    return forward_and_record(RecordType::click, RecordClick { x, y }, [&]() { return inner_->post_click(x, y, contact, pressure); });
}

MaaCtrlId ProxyController::post_swipe(int x1, int y1, int x2, int y2, int duration, int contact, int pressure)
{
    return forward_and_record(
        RecordType::swipe,
        RecordSwipe { x1, y1, x2, y2, duration },
        [&]() { return inner_->post_swipe(x1, y1, x2, y2, duration, contact, pressure); });
}

MaaCtrlId ProxyController::post_click_key(int keycode)
{
    return forward_and_record(RecordType::click_key, RecordKey { keycode }, [&]() { return inner_->post_click_key(keycode); });
}

MaaCtrlId ProxyController::post_input_text(const std::string& text)
{
    return forward_and_record(RecordType::input_text, RecordInputText { text }, [&]() { return inner_->post_input_text(text); });
}

MaaCtrlId ProxyController::post_start_app(const std::string& intent)
{
    return forward_and_record(RecordType::start_app, RecordApp { intent }, [&]() { return inner_->post_start_app(intent); });
}

MaaCtrlId ProxyController::post_stop_app(const std::string& intent)
{
    return forward_and_record(RecordType::stop_app, RecordApp { intent }, [&]() { return inner_->post_stop_app(intent); });
}

MaaCtrlId ProxyController::post_screencap()
{
    auto start = std::chrono::steady_clock::now();
    auto id = inner_->post_screencap();
    auto wait_status = inner_->wait(id);
    auto end = std::chrono::steady_clock::now();

    bool success = (wait_status == MaaStatus_Succeeded);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordScreencap param;

    if (success) {
        auto image = inner_->cached_image();
        if (!image.empty()) {
            std::unique_lock lock(recording_mutex_);
            param.path = std::format("screencap_{}.png", screencap_count_++);
            lock.unlock();

            MAA_NS::imwrite(dump_dir_ / param.path, image);
        }
    }

    write_record(make_record_json(make_line(RecordType::screencap, success, timestamp, static_cast<int>(cost)), param));
    return id;
}

MaaCtrlId ProxyController::post_touch_down(int contact, int x, int y, int pressure)
{
    return forward_and_record(
        RecordType::touch_down,
        RecordTouch { contact, x, y, pressure },
        [&]() { return inner_->post_touch_down(contact, x, y, pressure); });
}

MaaCtrlId ProxyController::post_touch_move(int contact, int x, int y, int pressure)
{
    return forward_and_record(
        RecordType::touch_move,
        RecordTouch { contact, x, y, pressure },
        [&]() { return inner_->post_touch_move(contact, x, y, pressure); });
}

MaaCtrlId ProxyController::post_touch_up(int contact)
{
    return forward_and_record(RecordType::touch_up, RecordTouch { contact }, [&]() { return inner_->post_touch_up(contact); });
}

MaaCtrlId ProxyController::post_key_down(int keycode)
{
    return forward_and_record(RecordType::key_down, RecordKey { keycode }, [&]() { return inner_->post_key_down(keycode); });
}

MaaCtrlId ProxyController::post_key_up(int keycode)
{
    return forward_and_record(RecordType::key_up, RecordKey { keycode }, [&]() { return inner_->post_key_up(keycode); });
}

MaaCtrlId ProxyController::post_scroll(int dx, int dy)
{
    return forward_and_record(RecordType::scroll, RecordScroll { dx, dy }, [&]() { return inner_->post_scroll(dx, dy); });
}

MaaCtrlId ProxyController::post_shell(const std::string& cmd, int64_t timeout)
{
    return inner_->post_shell(cmd, timeout);
}

MaaStatus ProxyController::status(MaaCtrlId ctrl_id) const
{
    return inner_->status(ctrl_id);
}

MaaStatus ProxyController::wait(MaaCtrlId ctrl_id) const
{
    return inner_->wait(ctrl_id);
}

bool ProxyController::connected() const
{
    return inner_->connected();
}

bool ProxyController::running() const
{
    return inner_->running();
}

cv::Mat ProxyController::cached_image() const
{
    return inner_->cached_image();
}

std::string ProxyController::cached_shell_output() const
{
    return inner_->cached_shell_output();
}

std::string ProxyController::get_uuid()
{
    return inner_->get_uuid();
}

bool ProxyController::get_resolution(int32_t& width, int32_t& height) const
{
    return inner_->get_resolution(width, height);
}

MaaSinkId ProxyController::add_sink(MaaEventCallback callback, void* trans_arg)
{
    return notifier_.add_sink(callback, trans_arg);
}

void ProxyController::remove_sink(MaaSinkId sink_id)
{
    notifier_.remove_sink(sink_id);
}

void ProxyController::clear_sinks()
{
    notifier_.clear_sinks();
}

void ProxyController::write_record(const json::value& record)
{
    std::unique_lock lock(recording_mutex_);

    if (!record_file_.is_open()) {
        LogError << "Recording file is not open";
        return;
    }

    record_file_ << record.to_string() << "\n";
    record_file_.flush();
}

template <typename ParamT>
MaaCtrlId ProxyController::forward_and_record(RecordType type, const ParamT& param, std::function<MaaCtrlId()> post_fn)
{
    auto start = std::chrono::steady_clock::now();
    auto id = post_fn();
    auto wait_status = inner_->wait(id);
    auto end = std::chrono::steady_clock::now();

    bool success = (wait_status == MaaStatus_Succeeded);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    write_record(make_record_json(make_line(type, success, timestamp, static_cast<int>(cost)), param));
    return id;
}

RecordLine ProxyController::make_line(RecordType type, bool success, int64_t timestamp, int cost)
{
    return RecordLine { .type = type, .timestamp = timestamp, .success = success, .cost = cost };
}

void MAA_CALL ProxyController::inner_sink_callback(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    auto* self = static_cast<ProxyController*>(trans_arg);
    if (!self) {
        return;
    }
    self->notifier_.notify(handle, message, json::parse(details_json).value_or(json::value()));
}

MAA_CTRL_NS_END
