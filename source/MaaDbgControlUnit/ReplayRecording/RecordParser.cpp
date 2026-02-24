#include "RecordParser.h"

#include <fstream>
#include <unordered_map>

#include <meojson/json.hpp>

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<Recording> RecordParser::parse(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    if (!std::filesystem::exists(path)) {
        LogError << "File not found:" << path;
        return std::nullopt;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        LogError << "Failed to open file:" << path;
        return std::nullopt;
    }

    Recording recording;

    std::string line;
    auto dir = path.parent_path();
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }

        auto json_opt = json::parse(line);
        if (!json_opt) {
            LogError << "Failed to parse json:" << line;
            return std::nullopt;
        }

        auto& record_json = json_opt.value();
        auto record_opt = parse_record(record_json, dir);
        if (!record_opt) {
            LogError << "Failed to parse record:" << line;
            return std::nullopt;
        }

        auto& record = record_opt.value();

        if (record.action.type == Record::Action::Type::connect) {
            auto& param = std::get<Record::ConnectParam>(record.action.param);
            recording.version = param.version;
            recording.device_info.uuid = param.uuid;
            recording.device_info.resolution = param.resolution;
        }

        recording.records.emplace_back(std::move(record_opt.value()));
    }

    return recording;
}

std::optional<Record> RecordParser::parse_record(const json::value& record_json, const std::filesystem::path& dir)
{
    Record record;
    record.raw_data = record_json;
    record.timestamp = record_json.get("timestamp", 0);
    record.success = record_json.get("success", false);
    record.cost = record_json.get("cost", 0);

    std::string type_str = record_json.get("type", std::string());
    static const std::unordered_map<std::string, Record::Action::Type> kTypeMap = {
        { "connect", Record::Action::Type::connect },           { "click", Record::Action::Type::click },
        { "swipe", Record::Action::Type::swipe },               { "multi_swipe", Record::Action::Type::multi_swipe },
        { "touch_down", Record::Action::Type::touch_down },     { "touch_move", Record::Action::Type::touch_move },
        { "touch_up", Record::Action::Type::touch_up },         { "click_key", Record::Action::Type::click_key },
        { "input_text", Record::Action::Type::input_text },     { "screencap", Record::Action::Type::screencap },
        { "start_app", Record::Action::Type::start_app },       { "stop_app", Record::Action::Type::stop_app },
        { "key_down", Record::Action::Type::key_down },         { "key_up", Record::Action::Type::key_up },
        { "scroll", Record::Action::Type::scroll },
    };

    auto it = kTypeMap.find(type_str);
    if (it == kTypeMap.end()) {
        LogError << "Invalid type:" << type_str;
        return std::nullopt;
    }

    record.action.type = it->second;

    std::optional<Record::Param> action_opt;
    switch (record.action.type) {
    case Record::Action::Type::connect:
        action_opt = parse_connect(record_json);
        break;
    case Record::Action::Type::click:
        action_opt = parse_click(record_json);
        break;
    case Record::Action::Type::swipe:
        action_opt = parse_swipe(record_json);
        break;
    case Record::Action::Type::multi_swipe:
        action_opt = parse_multi_swipe(record_json);
        break;
    case Record::Action::Type::touch_down:
    case Record::Action::Type::touch_move:
    case Record::Action::Type::touch_up:
        action_opt = parse_touch(record_json);
        break;
    case Record::Action::Type::click_key:
    case Record::Action::Type::key_down:
    case Record::Action::Type::key_up:
        action_opt = parse_click_key(record_json);
        break;
    case Record::Action::Type::input_text:
        action_opt = parse_input_text(record_json);
        break;
    case Record::Action::Type::screencap:
        action_opt = parse_screencap(record_json, dir);
        break;
    case Record::Action::Type::start_app:
    case Record::Action::Type::stop_app:
        action_opt = parse_app(record_json);
        break;
    case Record::Action::Type::scroll:
        action_opt = parse_scroll(record_json);
        break;
    default:
        LogError << "Invalid type:" << VAR(record.action.type);
        return std::nullopt;
    }

    if (!action_opt) {
        LogError << "Failed to parse action:" << VAR(type_str) << VAR(record_json);
        return std::nullopt;
    }

    record.action.param = std::move(action_opt.value());
    return record;
}

std::optional<Record::Param> RecordParser::parse_connect(const json::value& record_json)
{
    Record::ConnectParam result;

    if (auto version_opt = record_json.find<std::string>("version")) {
        result.version = *version_opt;
    }
    else {
        LogError << "Failed to find version:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto uuid_opt = record_json.find<std::string>("uuid")) {
        result.uuid = *uuid_opt;
    }
    else {
        LogError << "Failed to find uuid:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto w_opt = record_json.find<int>("width")) {
        result.resolution.width = *w_opt;
    }
    if (auto h_opt = record_json.find<int>("height")) {
        result.resolution.height = *h_opt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_click(const json::value& record_json)
{
    Record::ClickParam result;

    if (auto x_opt = record_json.find<int>("x")) {
        result.x = *x_opt;
    }
    else {
        LogError << "Failed to find x:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto y_opt = record_json.find<int>("y")) {
        result.y = *y_opt;
    }
    else {
        LogError << "Failed to find y:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_swipe(const json::value& record_json)
{
    Record::SwipeParam result;

    if (auto x1_opt = record_json.find<int>("x1")) {
        result.x1 = *x1_opt;
    }
    else {
        LogError << "Failed to find x1:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto y1_opt = record_json.find<int>("y1")) {
        result.y1 = *y1_opt;
    }
    else {
        LogError << "Failed to find y1:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto x2_opt = record_json.find<int>("x2")) {
        result.x2 = *x2_opt;
    }
    else {
        LogError << "Failed to find x2:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto y2_opt = record_json.find<int>("y2")) {
        result.y2 = *y2_opt;
    }
    else {
        LogError << "Failed to find y2:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto duration_opt = record_json.find<int>("duration")) {
        result.duration = *duration_opt;
    }
    else {
        LogError << "Failed to find duration:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_multi_swipe(const json::value& record_json)
{
    auto swipes_opt = record_json.find<json::array>("swipes");
    if (!swipes_opt) {
        LogError << "Failed to find swipes:" << VAR(record_json);
        return std::nullopt;
    }

    std::vector<Record::SwipeParam> result;
    for (const json::value& swipe : *swipes_opt) {
        auto s_opt = parse_swipe(swipe);
        if (!s_opt) {
            return std::nullopt;
        }
        auto s = std::get<Record::SwipeParam>(*std::move(s_opt));
        result.emplace_back(std::move(s));
    }
    return result;
}

std::optional<Record::Param> RecordParser::parse_touch(const json::value& record_json)
{
    Record::TouchParam result;

    if (auto contact_opt = record_json.find<int>("contact")) {
        result.contact = *contact_opt;
    }
    else {
        LogError << "Failed to find contact:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto x_opt = record_json.find<int>("x")) {
        result.x = *x_opt;
    }
    else {
        LogError << "Failed to find x:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto y_opt = record_json.find<int>("y")) {
        result.y = *y_opt;
    }
    else {
        LogError << "Failed to find y:" << VAR(record_json);
        return std::nullopt;
    }

    if (auto pressure_opt = record_json.find<int>("pressure")) {
        result.pressure = *pressure_opt;
    }
    else {
        LogError << "Failed to find pressure:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_click_key(const json::value& record_json)
{
    Record::ClickKeyParam result;

    if (auto keycode_opt = record_json.find<int>("keycode")) {
        result.keycode = *keycode_opt;
    }
    else {
        LogError << "Failed to find keycode:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_input_text(const json::value& record_json)
{
    Record::InputTextParam result;

    if (auto text_opt = record_json.find<std::string>("input_text")) {
        result.text = *text_opt;
    }
    else {
        LogError << "Failed to find text:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_screencap(const json::value& record_json, const std::filesystem::path& dir)
{
    Record::ScreencapParam result;

    if (auto path_opt = record_json.find<std::string>("path")) {
        result.path = MAA_NS::path(*path_opt);
    }
    else {
        LogError << "Failed to find path:" << VAR(record_json);
        return std::nullopt;
    }

    auto fullpath = dir / result.path;
    if (!std::filesystem::exists(fullpath)) {
        LogError << "File not found:" << VAR(fullpath);
        return std::nullopt;
    }

    result.image = imread(fullpath);

    return result;
}

std::optional<Record::Param> RecordParser::parse_app(const json::value& record_json)
{
    Record::AppParam result;

    if (auto package_opt = record_json.find<std::string>("package")) {
        result.package = *package_opt;
    }
    else {
        LogError << "Failed to find package:" << VAR(record_json);
        return std::nullopt;
    }

    return result;
}

std::optional<Record::Param> RecordParser::parse_scroll(const json::value& record_json)
{
    Record::ScrollParam result;

    if (auto dx_opt = record_json.find<int>("dx")) {
        result.dx = *dx_opt;
    }
    if (auto dy_opt = record_json.find<int>("dy")) {
        result.dy = *dy_opt;
    }

    return result;
}

MAA_CTRL_UNIT_NS_END
