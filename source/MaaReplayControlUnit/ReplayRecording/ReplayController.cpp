#include "ReplayController.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayController::ReplayController(Recording recording)
    : recording_(std::move(recording))
{
}

ReplayController::~ReplayController()
{
    // Intentional abort: ReplayController is designed for deterministic pipeline testing.
    // If the task ends early, it means the pipeline behavior diverged from the recording,
    // which is a test failure that should be caught immediately.
    if (record_index_ != recording_.records.size()) {
        LogError << "Failed to reproduce, the task ended early!" << VAR(record_index_) << VAR(recording_.records.size());
        std::abort();
    }
}

const Record* ReplayController::expect_record(RecordType expected_type)
{
    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return nullptr;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != expected_type) {
        LogError << "record type mismatch, expected" << VAR(expected_type) << "got" << VAR(record.action.type) << VAR(record.raw_data);
        return nullptr;
    }

    return &record;
}

bool ReplayController::consume_record(const Record& record)
{
    LogDebug << "sleep" << VAR(record.cost);
    std::this_thread::sleep_for(std::chrono::milliseconds(record.cost));
    ++record_index_;
    return record.success;
}

template <typename T>
const T* ReplayController::get_param(const Record& record)
{
    auto* p = std::get_if<T>(&record.action.param);
    if (!p) {
        LogError << "variant type mismatch in record" << VAR(record.raw_data);
    }
    return p;
}

bool ReplayController::connect()
{
    LogInfo;

    connected_ = false;

    auto* record = expect_record(RecordType::connect);
    if (!record) {
        return false;
    }

    connected_ = record->success;
    return consume_record(*record);
}

bool ReplayController::connected() const
{
    return connected_;
}

bool ReplayController::request_uuid(std::string& uuid)
{
    uuid = recording_.device_info.uuid;
    return true;
}

MaaControllerFeature ReplayController::get_features() const
{
    return MaaControllerFeature_None;
}

bool ReplayController::start_app(const std::string& intent)
{
    LogInfo << VAR(intent);

    auto* record = expect_record(RecordType::start_app);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordApp>(*record);
    if (!param) {
        return false;
    }

    if (param->package != intent) {
        LogError << "record intent mismatch" << VAR(param->package) << VAR(intent) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::stop_app(const std::string& intent)
{
    LogInfo << VAR(intent);

    auto* record = expect_record(RecordType::stop_app);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordApp>(*record);
    if (!param) {
        return false;
    }

    if (param->package != intent) {
        LogError << "record intent mismatch" << VAR(param->package) << VAR(intent) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::screencap(cv::Mat& image)
{
    LogInfo;

    auto* record = expect_record(RecordType::screencap);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordScreencapData>(*record);
    if (!param) {
        return false;
    }

    bool ret = consume_record(*record);

    image = record->success ? param->image : cv::Mat();
    return ret;
}

bool ReplayController::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    // TODO: 现在点击的点是随机区域，没法直接检查
    auto* record = expect_record(RecordType::click);
    if (!record) {
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    // TODO: 现在点击的点是随机区域，没法直接检查
    auto* record = expect_record(RecordType::swipe);
    if (!record) {
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    auto* record = expect_record(RecordType::touch_down);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordTouch>(*record);
    if (!param) {
        return false;
    }

    if (param->contact != contact || param->x != x || param->y != y || param->pressure != pressure) {
        LogError << "record touch_down mismatch" << VAR(param->contact) << VAR(param->x) << VAR(param->y) << VAR(param->pressure)
                 << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    auto* record = expect_record(RecordType::touch_move);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordTouch>(*record);
    if (!param) {
        return false;
    }

    if (param->contact != contact || param->x != x || param->y != y || param->pressure != pressure) {
        LogError << "record touch_move mismatch" << VAR(param->contact) << VAR(param->x) << VAR(param->y) << VAR(param->pressure)
                 << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::touch_up(int contact)
{
    LogInfo << VAR(contact);

    auto* record = expect_record(RecordType::touch_up);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordTouch>(*record);
    if (!param) {
        return false;
    }

    if (param->contact != contact) {
        LogError << "record touch_up mismatch" << VAR(param->contact) << VAR(contact) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::click_key(int key)
{
    LogInfo << VAR(key);

    auto* record = expect_record(RecordType::click_key);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordKey>(*record);
    if (!param) {
        return false;
    }

    if (param->keycode != key) {
        LogError << "record click_key mismatch" << VAR(param->keycode) << VAR(key) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    auto* record = expect_record(RecordType::input_text);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordInputText>(*record);
    if (!param) {
        return false;
    }

    if (param->text != text) {
        LogError << "record input_text mismatch" << VAR(param->text) << VAR(text) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::key_down(int key)
{
    LogInfo << VAR(key);

    auto* record = expect_record(RecordType::key_down);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordKey>(*record);
    if (!param) {
        return false;
    }

    if (param->keycode != key) {
        LogError << "record key_down mismatch" << VAR(param->keycode) << VAR(key) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::key_up(int key)
{
    LogInfo << VAR(key);

    auto* record = expect_record(RecordType::key_up);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordKey>(*record);
    if (!param) {
        return false;
    }

    if (param->keycode != key) {
        LogError << "record key_up mismatch" << VAR(param->keycode) << VAR(key) << VAR(record->raw_data);
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    auto* record = expect_record(RecordType::scroll);
    if (!record) {
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::relative_move(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    auto* record = expect_record(RecordType::relative_move);
    if (!record) {
        return false;
    }

    return consume_record(*record);
}

bool ReplayController::shell(const std::string& cmd, std::string& output, std::chrono::milliseconds timeout)
{
    LogInfo << VAR(cmd) << VAR(timeout.count());

    auto* record = expect_record(RecordType::shell);
    if (!record) {
        return false;
    }

    auto* param = get_param<RecordShell>(*record);
    if (!param) {
        return false;
    }

    if (param->cmd != cmd) {
        LogError << "record shell cmd mismatch" << VAR(param->cmd) << VAR(cmd) << VAR(record->raw_data);
        return false;
    }

    bool ret = consume_record(*record);

    if (record->success) {
        output = param->output;
    }

    return ret;
}

bool ReplayController::inactive()
{
    return true;
}

json::object ReplayController::get_info() const
{
    json::object info;
    info["type"] = "replay";
    info["record_count"] = static_cast<int64_t>(recording_.records.size());
    info["record_index"] = static_cast<int64_t>(record_index_);
    return info;
}

MAA_CTRL_UNIT_NS_END
