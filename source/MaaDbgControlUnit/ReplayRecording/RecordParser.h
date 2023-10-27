#pragma once

#include <filesystem>

#include <meojson/json.hpp>

#include "Record.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

class RecordParser
{
public:
    static std::optional<Recording> parse(const std::filesystem::path& path);

private:
    static std::optional<Record> parse_record(const json::value& record_json);
};

MAA_DBG_CTRL_UNIT_NS_END
