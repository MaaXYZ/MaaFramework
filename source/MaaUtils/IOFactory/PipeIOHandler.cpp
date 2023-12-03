#include "PipeIOHandler.h"

MAA_NS_BEGIN

PipeIOHandler::PipeIOHandler(const std::filesystem::path& exec, const std::vector<std::string>& args)
    : child_(exec, args, //
             boost::process::std_in<pin_, boost::process::std_out> pout_, boost::process::std_err > pout_
#ifdef _WIN32
             ,
             boost::process::windows::create_no_window
#endif
             ),
      buffer_(std::make_unique<char[]>(kBufferSize))
{}

PipeIOHandler::~PipeIOHandler()
{
    if (child_.running()) {
        child_.terminate();
    }
    else {
        child_.wait();
    }
}

bool PipeIOHandler::write(const std::string& data)
{
    if (!child_.running()) {
        return false;
    }

    pin_ << data << std::endl;
    return true;
}

std::string PipeIOHandler::read(std::chrono::milliseconds timeout, size_t max_length)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        return std::chrono::steady_clock::now() - start_time < timeout;
    };

    std::string result;

    while (max_length > result.size() && check_timeout(start_time)) {
        auto read_size = std::min(kBufferSize, max_length - result.size());
        auto read_num = pout_.readsome(buffer_.get(), read_size);
        if (read_num > 0) {
            result.append(buffer_.get(), read_num);
        }
    }

    return result;
}

MAA_NS_END
