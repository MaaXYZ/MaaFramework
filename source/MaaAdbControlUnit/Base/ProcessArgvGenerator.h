#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Utils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ProcessArgvGenerator
{
public:
    using Replacement = std::unordered_map<std::string, std::string>;

    struct ProcessArgv
    {
        std::filesystem::path exec;
        std::vector<std::string> args;
    };

public:
    static std::optional<ProcessArgvGenerator> create(const json::array& arr);

    ProcessArgvGenerator() = default;

    explicit ProcessArgvGenerator(std::vector<std::string> raw)
        : raw_(std::move(raw))
    {
    }

    std::optional<ProcessArgv> gen(const Replacement& replacement) const;

private:
    std::vector<std::string> raw_;
};

MAA_CTRL_UNIT_NS_END
