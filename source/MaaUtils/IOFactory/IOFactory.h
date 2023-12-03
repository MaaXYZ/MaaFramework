#pragma once

#include <filesystem>
#include <memory>

#include "IOHandler.h"

MAA_NS_BEGIN

class IOFactory
{
public:
    static std::shared_ptr<IOHandler> child_pipe_io( //
        const std::filesystem::path& exec, const std::vector<std::string>& args);

    static std::shared_ptr<IOHandler> child_sock_io( //
        const std::filesystem::path& exec, const std::vector<std::string>& args);

    static std::shared_ptr<IOHandler> tcp_io( //
        const std::string& host, unsigned short port);
};

MAA_NS_END
