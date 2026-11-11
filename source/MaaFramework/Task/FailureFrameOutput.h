#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

#include "Common/Conf.h"

MAA_NS_BEGIN

// Owns the directory handles for a single failure frame export.
class FailureFrameOutput
{
public:
    FailureFrameOutput(const std::filesystem::path& directory, const std::filesystem::path& temporary);
    ~FailureFrameOutput();

    void write_frame(const std::filesystem::path& name, std::span<const std::byte> bytes);
    void write_manifest(std::string_view data);
    void publish();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

MAA_NS_END
