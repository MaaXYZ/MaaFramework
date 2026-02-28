#pragma once

#include <filesystem>

#include "Record.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class RecordParser
{
public:
    static std::optional<Recording> parse(const std::filesystem::path& path);

private:
    static std::optional<Record> parse_record(const json::value& record_json, const std::filesystem::path& dir);
};

MAA_CTRL_UNIT_NS_END
