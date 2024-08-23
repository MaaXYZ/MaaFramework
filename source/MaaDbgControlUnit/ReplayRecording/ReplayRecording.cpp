#include "ReplayRecording.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ReplayRecording::~ReplayRecording()
{
    if (record_index_ != recording_.records.size()) {
        LogError << "Failed to reproduce, the task ended early!" << VAR(record_index_) << VAR(recording_.records.size());
        std::abort();
    }
}

bool ReplayRecording::find_device(std::vector<std::string>& devices)
{
    std::ignore = devices;
    return true;
}

bool ReplayRecording::connect()
{
    LogInfo;

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != Record::Action::Type::connect) {
        LogError << "record type is not connect" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::request_uuid(std::string& uuid)
{
    uuid = recording_.device_info.uuid;
    return true;
}

bool ReplayRecording::start_app(const std::string& intent)
{
    LogInfo << VAR(intent);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != Record::Action::Type::start_app) {
        LogError << "record type is not start" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::AppParam>(record.action.param);

    if (param.package != intent) {
        LogError << "record intent is not match" << VAR(param.package) << VAR(intent) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::stop_app(const std::string& intent)
{
    LogInfo << VAR(intent);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != Record::Action::Type::stop_app) {
        LogError << "record type is not stop" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::AppParam>(record.action.param);

    if (param.package != intent) {
        LogError << "record intent is not match" << VAR(param.package) << VAR(intent) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::screencap(cv::Mat& image)
{
    LogInfo;

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::screencap) {
        LogError << "record type is not screencap" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::ScreencapParam>(record.action.param);

    sleep(record.cost);
    ++record_index_;

    image = record.success ? param.image : cv::Mat();
    return true;
}

bool ReplayRecording::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != Record::Action::Type::click) {
        LogError << "record type is not click" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    // TODO: 现在点击的点是随机区域，没法直接检查
    // auto param = std::get<Record::ClickParam>(record.action.param);

    // if (param.x != x || param.y != y) {
    //     LogError << "record click is not match" << VAR(param.x) << VAR(param.y) << VAR(x) <<
    //     VAR(y)
    //              << VAR(record.raw_data);
    //     return false;
    // }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);
    if (record.action.type != Record::Action::Type::swipe) {
        LogError << "record type is not swipe" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    // TODO: 现在点击的点是随机区域，没法直接检查
    // auto param = std::get<Record::SwipeParam>(record.action.param);

    // if (param.x1 != x1 || param.y1 != y1 || param.x2 != x2 || param.y2 != y2 || param.duration !=
    // duration) {
    //     LogError << "record swipe is not match" << VAR(param.x1) << VAR(param.y1) <<
    //     VAR(param.x2) << VAR(param.y2)
    //              << VAR(param.duration) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) <<
    //              VAR(duration)
    //              << VAR(record.raw_data);
    //     return false;
    // }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::touch_down) {
        LogError << "record type is not touch_down" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::TouchParam>(record.action.param);

    if (param.contact != contact || param.x != x || param.y != y || param.pressure != pressure) {
        LogError << "record touch_down is not match" << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure)
                 << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::touch_move) {
        LogError << "record type is not touch_move" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::TouchParam>(record.action.param);

    if (param.contact != contact || param.x != x || param.y != y || param.pressure != pressure) {
        LogError << "record touch_move is not match" << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure)
                 << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::touch_up(int contact)
{
    LogInfo << VAR(contact);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::touch_up) {
        LogError << "record type is not touch_up" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::TouchParam>(record.action.param);

    if (param.contact != contact) {
        LogError << "record touch_up is not match" << VAR(param.contact) << VAR(contact) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::press_key(int key)
{
    LogInfo << VAR(key);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::press_key) {
        LogError << "record type is not press_key" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::PressKeyParam>(record.action.param);

    if (param.keycode != key) {
        LogError << "record press_key is not match" << VAR(param.keycode) << VAR(key) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

bool ReplayRecording::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (record_index_ >= recording_.records.size()) {
        LogError << "record index out of range" << VAR(record_index_) << VAR(recording_.records.size());
        return false;
    }

    const Record& record = recording_.records.at(record_index_);

    if (record.action.type != Record::Action::Type::input_text) {
        LogError << "record type is not input_text" << VAR(record.action.type) << VAR(record.raw_data);
        return false;
    }

    auto param = std::get<Record::InputTextParam>(record.action.param);

    if (param.text != text) {
        LogError << "record text is not match" << VAR(param.text) << VAR(text) << VAR(record.raw_data);
        return false;
    }

    sleep(record.cost);
    ++record_index_;
    return record.success;
}

void ReplayRecording::sleep(int ms)
{
    LogTrace << VAR(ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

MAA_CTRL_UNIT_NS_END
