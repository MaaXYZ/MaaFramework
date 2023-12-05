#include "Utils/IOStream/ChildPipeIOStream.h"

MAA_NS_BEGIN

ChildPipeIOStream::ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args)
    : child_( //
          exec, args, boost::process::std_out > pout_, boost::process::std_err > pout_, boost::process::std_in < pin_
#ifdef _WIN32
          ,
          boost::process::windows::create_no_window
#endif
          ),
      buffer_(std::make_unique<char[]>(kBufferSize))
{}

ChildPipeIOStream::~ChildPipeIOStream()
{
    release();
}

bool ChildPipeIOStream::write(std::string_view data)
{
    if (!child_.running()) {
        return false;
    }

    pin_ << data << std::endl;
    return true;
}

std::string ChildPipeIOStream::read(std::chrono::seconds timeout, size_t count)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        return timeout > std::chrono::seconds(0) && std::chrono::steady_clock::now() - start_time < timeout;
    };

    std::string result;

    while (check_timeout(start_time) && count > result.size() && child_.running()) {
        auto read_size = std::min(kBufferSize, count - result.size());
        auto read_num = pout_.readsome(buffer_.get(), read_size);
        if (read_num > 0) {
            result.append(buffer_.get(), read_num);
        }
    }

    return result;
}

int ChildPipeIOStream::release()
{
    if (child_.running()) {
        child_.terminate();
    }
    else {
        child_.wait();
    }

    return child_.exit_code();
}

MAA_NS_END
