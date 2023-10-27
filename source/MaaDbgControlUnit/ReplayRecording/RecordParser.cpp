#include "RecordParser.h"

#include <fstream>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Utils/Logger.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

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
        auto record_opt = parse_record(record_json);
        if (!record_opt) {
            LogError << "Failed to parse record:" << line;
            return std::nullopt;
        }

        recording.records.emplace_back(std::move(record_opt.value()));
    }
}

std::optional<Record> RecordParser::parse_record(const json::value& record_json)
{
    Record record;
    record.timestamp = record_json.get("timestamp", 0);
    record.success = record_json.get("success", false);
    record.cost = record_json.get("cost", 0);

    std::string type_str = record_json.get("type", std::string());
    static const std::unordered_map<std::string, Record::Action::Type> kTypeMap = {
        { "connect", Record::Action::Type::connect },       { "click", Record::Action::Type::click },
        { "swipe", Record::Action::Type::swipe },           { "touch_down", Record::Action::Type::touch_down },
        { "touch_move", Record::Action::Type::touch_move }, { "touch_up", Record::Action::Type::touch_up },
        { "press_key", Record::Action::Type::press_key },   { "screencap", Record::Action::Type::screencap },
        { "start_app", Record::Action::Type::start_app },   { "stop_app", Record::Action::Type::stop_app },
    };

    auto it = kTypeMap.find(type_str);
    if (it == kTypeMap.end()) {
        LogError << "Invalid type:" << type_str;
        return std::nullopt;
    }

    record.action.type = it->second;
}

MAA_DBG_CTRL_UNIT_NS_END
