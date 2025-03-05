#pragma once

#include <filesystem>
#include <format>
#include <string>

#include "Conf/Conf.h"
#include "Utils/Platform.h"

MAA_AGENT_NS_BEGIN

inline std::string generate_socket_address(const std::string& identifier)
{
    static auto kTempDir = std::filesystem::temp_directory_path();
    constexpr std::string_view kAddrFormat = "ipc://{}/maafw-agent-{}.sock";

    return std::format(kAddrFormat, path_to_utf8_string(kTempDir), identifier);
}

MAA_AGENT_NS_END
