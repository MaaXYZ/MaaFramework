#include "ProxyController.h"

#include <format>

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ProxyController::ProxyController(std::shared_ptr<ControlUnitAPI> inner, std::filesystem::path dump_dir)
    : inner_(std::move(inner))
    , dump_dir_(std::move(dump_dir))
    , recording_start_(std::chrono::steady_clock::now())
{
    LogFunc << VAR_VOIDP(inner_.get()) << VAR(dump_dir_);

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
}

ProxyController::~ProxyController()
{
    LogFunc;

    if (record_file_.is_open()) {
        record_file_.flush();
        record_file_.close();
    }
}

bool ProxyController::connect()
{
    auto start = std::chrono::steady_clock::now();
    bool success = inner_->connect();
    auto end = std::chrono::steady_clock::now();

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordConnect param;
    param.version = MAA_VERSION;
    if (success) {
        inner_->request_uuid(param.uuid);
    }

    write_record(make_record_json(make_line(RecordType::connect, success, timestamp, static_cast<int>(cost)), param));
    return success;
}

bool ProxyController::connected() const
{
    return inner_->connected();
}

bool ProxyController::request_uuid(std::string& uuid)
{
    return inner_->request_uuid(uuid);
}

MaaControllerFeature ProxyController::get_features() const
{
    return inner_->get_features();
}

bool ProxyController::start_app(const std::string& intent)
{
    return forward_and_record(RecordType::start_app, RecordApp { intent }, [&]() { return inner_->start_app(intent); });
}

bool ProxyController::stop_app(const std::string& intent)
{
    return forward_and_record(RecordType::stop_app, RecordApp { intent }, [&]() { return inner_->stop_app(intent); });
}

bool ProxyController::screencap(cv::Mat& image)
{
    auto start = std::chrono::steady_clock::now();
    bool success = inner_->screencap(image);
    auto end = std::chrono::steady_clock::now();

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordScreencap param;

    if (success && !image.empty()) {
        std::unique_lock lock(recording_mutex_);
        param.path = std::format("screencap_{}.png", screencap_count_++);
        lock.unlock();

        MAA_NS::imwrite(dump_dir_ / param.path, image);
    }

    write_record(make_record_json(make_line(RecordType::screencap, success, timestamp, static_cast<int>(cost)), param));
    return success;
}

bool ProxyController::click(int x, int y)
{
    return forward_and_record(RecordType::click, RecordClick { x, y }, [&]() { return inner_->click(x, y); });
}

bool ProxyController::swipe(int x1, int y1, int x2, int y2, int duration)
{
    return forward_and_record(
        RecordType::swipe,
        RecordSwipe { x1, y1, x2, y2, duration },
        [&]() { return inner_->swipe(x1, y1, x2, y2, duration); });
}

bool ProxyController::touch_down(int contact, int x, int y, int pressure)
{
    return forward_and_record(
        RecordType::touch_down,
        RecordTouch { contact, x, y, pressure },
        [&]() { return inner_->touch_down(contact, x, y, pressure); });
}

bool ProxyController::touch_move(int contact, int x, int y, int pressure)
{
    return forward_and_record(
        RecordType::touch_move,
        RecordTouch { contact, x, y, pressure },
        [&]() { return inner_->touch_move(contact, x, y, pressure); });
}

bool ProxyController::touch_up(int contact)
{
    return forward_and_record(RecordType::touch_up, RecordTouch { contact }, [&]() { return inner_->touch_up(contact); });
}

bool ProxyController::click_key(int key)
{
    return forward_and_record(RecordType::click_key, RecordKey { key }, [&]() { return inner_->click_key(key); });
}

bool ProxyController::input_text(const std::string& text)
{
    return forward_and_record(RecordType::input_text, RecordInputText { text }, [&]() { return inner_->input_text(text); });
}

bool ProxyController::key_down(int key)
{
    return forward_and_record(RecordType::key_down, RecordKey { key }, [&]() { return inner_->key_down(key); });
}

bool ProxyController::key_up(int key)
{
    return forward_and_record(RecordType::key_up, RecordKey { key }, [&]() { return inner_->key_up(key); });
}

bool ProxyController::scroll(int dx, int dy)
{
    return forward_and_record(RecordType::scroll, RecordScroll { dx, dy }, [&]() { return inner_->scroll(dx, dy); });
}

bool ProxyController::find_device(std::vector<std::string>& devices)
{
    auto adb = std::dynamic_pointer_cast<AdbControlUnitAPI>(inner_);
    if (!adb) {
        return false;
    }
    return adb->find_device(devices);
}

bool ProxyController::shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout)
{
    auto adb = std::dynamic_pointer_cast<AdbControlUnitAPI>(inner_);
    if (!adb) {
        return false;
    }
    return adb->shell(cmd, output, timeout);
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
bool ProxyController::forward_and_record(RecordType type, const ParamT& param, std::function<bool()> action_fn)
{
    auto start = std::chrono::steady_clock::now();
    bool success = action_fn();
    auto end = std::chrono::steady_clock::now();

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    write_record(make_record_json(make_line(type, success, timestamp, static_cast<int>(cost)), param));
    return success;
}

RecordLine ProxyController::make_line(RecordType type, bool success, int64_t timestamp, int cost)
{
    return RecordLine { .type = type, .timestamp = timestamp, .success = success, .cost = cost };
}

MAA_CTRL_UNIT_NS_END
