#include "ProcessArgvGenerator.h"

#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<ProcessArgvGenerator> ProcessArgvGenerator::create(const json::array& arr)
{
    if (!arr.all<std::string>()) {
        LogError << "array not all string" << VAR(arr);
        return std::nullopt;
    }

    auto raw = arr.as<std::vector<std::string>>();
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

    std::filesystem::path exec = boost::process::search_path(path(res.front()));
    if (!std::filesystem::exists(exec)) {
        LogError << "exec path not exists" << VAR(res.front()) << VAR(exec);
        return std::nullopt;
    }

    auto args = std::vector(std::make_move_iterator(res.begin() + 1), std::make_move_iterator(res.end()));

    return ProcessArgv { .exec = std::move(exec), .args = std::move(args) };
}

MAA_CTRL_UNIT_NS_END
