#pragma once

#include <filesystem>

#include <meojson/json.hpp>

#include "Record.h"

MAA_CTRL_UNIT_NS_BEGIN

class RecordParser
{
public:
    static std::optional<Recording> parse(const std::filesystem::path& path);

private:
    static std::optional<Record> parse_record(const json::value& record_json, const std::filesystem::path& dir);
    static std::optional<Record::Param> parse_connect(const json::value& record_json);
    static std::optional<Record::Param> parse_click(const json::value& record_json);
    static std::optional<Record::Param> parse_swipe(const json::value& record_json);
    static std::optional<Record::Param> parse_touch(const json::value& record_json);
    static std::optional<Record::Param> parse_press_key(const json::value& record_json);
    static std::optional<Record::Param> parse_input_text(const json::value& record_json);
    static std::optional<Record::Param> parse_screencap(const json::value& record_json, const std::filesystem::path& dir);
    static std::optional<Record::Param> parse_app(const json::value& record_json);
};

MAA_CTRL_UNIT_NS_END
