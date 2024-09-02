#include "ProcessArgvGenerator.h"

#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<ProcessArgvGenerator> ProcessArgvGenerator::create(const json::array& arr)
{
    if (!arr.all<std::string>()) {
        LogError << "array not all string" << VAR(arr);
        return std::nullopt;
    }

    auto raw = arr.as_collection<std::string>();
    if (raw.empty()) {
        LogError << "array is empty";
        return std::nullopt;
    }

    return ProcessArgvGenerator(std::move(raw));
}

std::optional<ProcessArgvGenerator::ProcessArgv> ProcessArgvGenerator::gen(const Replacement& replacement) const
{
    if (raw_.empty()) {
        LogError << "raw is empty";
        return std::nullopt;
    }

    auto res = raw_;
    for (auto& s : res) {
        string_replace_all_(s, replacement);
    }

    std::filesystem::path abs_path;
    if (auto raw_path = MAA_NS::path(res.front()); raw_path.is_absolute()) {
        abs_path = raw_path;
    }
    else {
        abs_path = boost::process::search_path(raw_path);
    }

    if (!std::filesystem::exists(abs_path)) {
        LogError << "exec path not exists" << VAR(abs_path);
        return std::nullopt;
    }

    auto args = std::vector(std::make_move_iterator(res.begin() + 1), std::make_move_iterator(res.end()));

    return ProcessArgv { .exec = std::move(abs_path), .args = std::move(args) };
}

MAA_CTRL_UNIT_NS_END
