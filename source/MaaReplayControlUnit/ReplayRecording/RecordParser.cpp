#include "RecordParser.h"

#include <fstream>

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

        if (record.action.type == RecordType::connect) {
            auto& param = std::get<RecordConnect>(record.action.param);
            recording.version = param.version;
            recording.device_info.uuid = param.uuid;
            recording.device_info.resolution = cv::Size(param.width, param.height);
        }

        recording.records.emplace_back(std::move(record_opt.value()));
    }

    return recording;
}

std::optional<Record> RecordParser::parse_record(const json::value& record_json, const std::filesystem::path& dir)
{
    RecordLine line;
    if (!line.from_json(record_json)) {
        LogError << "Failed to parse record line:" << VAR(record_json);
        return std::nullopt;
    }

    if (line.type == RecordType::invalid) {
        LogError << "Invalid record type:" << VAR(record_json);
        return std::nullopt;
    }

    Record record;
    record.raw_data = record_json;
    record.timestamp = line.timestamp;
    record.success = line.success;
    record.cost = line.cost;
    record.action.type = line.type;

    auto parse_param = [&]<typename T>() -> std::optional<Record::Param> {
        T param;
        if (!param.from_json(record_json)) {
            LogError << "Failed to parse param:" << VAR(line.type) << VAR(record_json);
            return std::nullopt;
        }
        return param;
    };

    std::optional<Record::Param> param_opt;

    switch (line.type) {
    case RecordType::connect:
        param_opt = parse_param.operator()<RecordConnect>();
        break;
    case RecordType::click:
        param_opt = parse_param.operator()<RecordClick>();
        break;
    case RecordType::swipe:
        param_opt = parse_param.operator()<RecordSwipe>();
        break;
    case RecordType::multi_swipe: {
        RecordMultiSwipe ms;
        if (!ms.from_json(record_json)) {
            LogError << "Failed to parse multi_swipe:" << VAR(record_json);
            return std::nullopt;
        }
        param_opt = std::move(ms.swipes);
        break;
    }
    case RecordType::touch_down:
    case RecordType::touch_move:
    case RecordType::touch_up:
        param_opt = parse_param.operator()<RecordTouch>();
        break;
    case RecordType::click_key:
    case RecordType::key_down:
    case RecordType::key_up:
        param_opt = parse_param.operator()<RecordKey>();
        break;
    case RecordType::input_text:
        param_opt = parse_param.operator()<RecordInputText>();
        break;
    case RecordType::screencap: {
        RecordScreencapData sp;
        if (!sp.from_json(record_json)) {
            LogError << "Failed to parse screencap:" << VAR(record_json);
            return std::nullopt;
        }
        auto fullpath = dir / MAA_NS::path(sp.path);
        if (std::filesystem::exists(fullpath)) {
            sp.image = imread(fullpath);
        }
        else {
            LogError << "Screencap file not found:" << VAR(fullpath);
            return std::nullopt;
        }
        param_opt = std::move(sp);
        break;
    }
    case RecordType::start_app:
    case RecordType::stop_app:
        param_opt = parse_param.operator()<RecordApp>();
        break;
    case RecordType::scroll:
        param_opt = parse_param.operator()<RecordScroll>();
        break;
    default:
        LogError << "Unhandled record type:" << VAR(line.type);
        return std::nullopt;
    }

    if (!param_opt) {
        return std::nullopt;
    }

    record.action.param = std::move(param_opt.value());
    return record;
}

MAA_CTRL_UNIT_NS_END
