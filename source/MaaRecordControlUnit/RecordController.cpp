#include "RecordController.h"

#include <format>

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

RecordController::RecordController(std::shared_ptr<ControlUnitAPI> inner, std::filesystem::path recording_path)
    : inner_(std::move(inner))
    , recording_path_(std::move(recording_path))
    , recording_start_(std::chrono::steady_clock::now())
{
    LogFunc << VAR_VOIDP(inner_.get()) << VAR(recording_path_);

    auto parent_dir = recording_path_.parent_path();
    auto stem = recording_path_.stem().string();

    screenshot_dir_ = parent_dir / (stem + "-Screenshot");
    screenshot_rel_prefix_ = stem + "-Screenshot";

    std::error_code ec;
    std::filesystem::create_directories(parent_dir, ec);
    if (ec) {
        LogError << "Failed to create parent directory:" << parent_dir << VAR(ec.message());
    }

    std::filesystem::create_directories(screenshot_dir_, ec);
    if (ec) {
        LogError << "Failed to create screenshot directory:" << screenshot_dir_ << VAR(ec.message());
    }

    record_file_.open(recording_path_, std::ios::out | std::ios::trunc);
    if (!record_file_.is_open()) {
        LogError << "Failed to open recording file:" << recording_path_;
    }
}

RecordController::~RecordController()
{
    LogFunc;

    if (record_file_.is_open()) {
        record_file_.flush();
        record_file_.close();
    }
}

bool RecordController::connect()
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

bool RecordController::connected() const
{
    return inner_->connected();
}

bool RecordController::request_uuid(std::string& uuid)
{
    return inner_->request_uuid(uuid);
}

MaaControllerFeature RecordController::get_features() const
{
    return inner_->get_features();
}

bool RecordController::start_app(const std::string& intent)
{
    return forward_and_record(RecordType::start_app, RecordApp { intent }, [&]() { return inner_->start_app(intent); });
}

bool RecordController::stop_app(const std::string& intent)
{
    return forward_and_record(RecordType::stop_app, RecordApp { intent }, [&]() { return inner_->stop_app(intent); });
}

bool RecordController::screencap(cv::Mat& image)
{
    auto start = std::chrono::steady_clock::now();
    bool success = inner_->screencap(image);
    auto end = std::chrono::steady_clock::now();

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordScreencap param;

    if (success && !image.empty()) {
        std::unique_lock lock(recording_mutex_);
        auto filename = std::format("screencap_{}.png", screencap_count_++);
        param.path = screenshot_rel_prefix_ + "/" + filename;
        lock.unlock();

        MAA_NS::imwrite(screenshot_dir_ / filename, image);
    }

    write_record(make_record_json(make_line(RecordType::screencap, success, timestamp, static_cast<int>(cost)), param));
    return success;
}

bool RecordController::click(int x, int y)
{
    return forward_and_record(RecordType::click, RecordClick { x, y }, [&]() { return inner_->click(x, y); });
}

bool RecordController::swipe(int x1, int y1, int x2, int y2, int duration)
{
    return forward_and_record(RecordType::swipe, RecordSwipe { x1, y1, x2, y2, duration }, [&]() {
        return inner_->swipe(x1, y1, x2, y2, duration);
    });
}

bool RecordController::touch_down(int contact, int x, int y, int pressure)
{
    return forward_and_record(RecordType::touch_down, RecordTouch { contact, x, y, pressure }, [&]() {
        return inner_->touch_down(contact, x, y, pressure);
    });
}

bool RecordController::touch_move(int contact, int x, int y, int pressure)
{
    return forward_and_record(RecordType::touch_move, RecordTouch { contact, x, y, pressure }, [&]() {
        return inner_->touch_move(contact, x, y, pressure);
    });
}

bool RecordController::touch_up(int contact)
{
    return forward_and_record(RecordType::touch_up, RecordTouch { contact }, [&]() { return inner_->touch_up(contact); });
}

bool RecordController::click_key(int key)
{
    return forward_and_record(RecordType::click_key, RecordKey { key }, [&]() { return inner_->click_key(key); });
}

bool RecordController::input_text(const std::string& text)
{
    return forward_and_record(RecordType::input_text, RecordInputText { text }, [&]() { return inner_->input_text(text); });
}

bool RecordController::key_down(int key)
{
    return forward_and_record(RecordType::key_down, RecordKey { key }, [&]() { return inner_->key_down(key); });
}

bool RecordController::key_up(int key)
{
    return forward_and_record(RecordType::key_up, RecordKey { key }, [&]() { return inner_->key_up(key); });
}

bool RecordController::scroll(int dx, int dy)
{
    auto action = [&]() -> bool {
        if (auto p = std::dynamic_pointer_cast<ScrollableUnit>(inner_)) {
            return p->scroll(dx, dy);
        }
        LogError << "Inner controller does not support scroll";
        return false;
    };
    return forward_and_record(RecordType::scroll, RecordScroll { dx, dy }, action);
}

bool RecordController::relative_move(int dx, int dy)
{
    auto action = [&]() -> bool {
        if (auto p = std::dynamic_pointer_cast<RelativeMovableUnit>(inner_)) {
            return p->relative_move(dx, dy);
        }
        LogError << "Inner controller does not support relative_move";
        return false;
    };
    return forward_and_record(RecordType::relative_move, RecordRelativeMove { dx, dy }, action);
}

bool RecordController::inactive()
{
    return inner_->inactive();
}

json::object RecordController::get_info() const
{
    json::object info = inner_->get_info();
    info["recording"] = true;
    info["recording_path"] = MAA_NS::path_to_utf8_string(recording_path_);
    return info;
}

bool RecordController::shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout)
{
    auto start = std::chrono::steady_clock::now();

    bool success = false;
    if (auto p = std::dynamic_pointer_cast<ShellableUnit>(inner_)) {
        success = p->shell(cmd, output, timeout);
    }
    else {
        LogError << "Inner controller does not support shell";
    }

    auto end = std::chrono::steady_clock::now();

    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RecordShell param { .cmd = cmd, .timeout = timeout.count(), .output = success ? output : std::string() };
    write_record(make_record_json(make_line(RecordType::shell, success, ts, static_cast<int>(cost)), param));

    return success;
}

void RecordController::write_record(const json::value& record)
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
bool RecordController::forward_and_record(RecordType type, const ParamT& param, std::function<bool()> action_fn)
{
    auto start = std::chrono::steady_clock::now();
    bool success = action_fn();
    auto end = std::chrono::steady_clock::now();

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start - recording_start_).count();
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    write_record(make_record_json(make_line(type, success, timestamp, static_cast<int>(cost)), param));
    return success;
}

RecordLine RecordController::make_line(RecordType type, bool success, int64_t timestamp, int cost)
{
    return RecordLine { .type = type, .timestamp = timestamp, .success = success, .cost = cost };
}

MAA_CTRL_UNIT_NS_END
