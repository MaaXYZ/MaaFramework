#include "IOFactory.h"

#include "PipeIOHandler.h"
#include "Utils/Boost.hpp"
#include "Utils/Logger.h"

MAA_NS_BEGIN

std::shared_ptr<IOHandler> IOFactory::child_pipe_io(const std::filesystem::path& exec,
                                                    const std::vector<std::string>& args)
{
    auto searched_exec = boost::process::search_path(exec);
    if (!std::filesystem::exists(searched_exec)) {
        LogError << "exec not found" << exec;
        return nullptr;
    }

    return std::make_shared<PipeIOHandler>(searched_exec, args);
}

std::shared_ptr<IOHandler> IOFactory::child_sock_io(const std::filesystem::path& exec,
                                                    const std::vector<std::string>& args)
{
    return std::shared_ptr<IOHandler>();
}

std::shared_ptr<IOHandler> IOFactory::tcp_io(const std::string& host, unsigned short port)
{
    return std::shared_ptr<IOHandler>();
}

MAA_NS_END
