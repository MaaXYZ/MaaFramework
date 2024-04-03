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

std::optional<ProcessArgvGenerator::ProcessArgv>
    ProcessArgvGenerator::gen(const Replacement& replacement) const
{
    if (raw_.empty()) {
        LogError << "raw is empty";
        return std::nullopt;
    }

    auto res = raw_;
    for (auto& s : res) {
        string_replace_all_(s, replacement);
    }

    auto stdpath = MAA_NS::path(res.front());
    auto searched_path = boost::process::search_path(stdpath);

    if (!std::filesystem::exists(searched_path)) {
        LogError << "exec path not exits" << VAR(searched_path);
        return std::nullopt;
    }

    auto args =
        std::vector(std::make_move_iterator(res.begin() + 1), std::make_move_iterator(res.end()));

    return ProcessArgv { .exec = std::move(searched_path), .args = std::move(args) };
}

MAA_CTRL_UNIT_NS_END