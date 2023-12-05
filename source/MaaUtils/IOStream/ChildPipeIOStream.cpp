#include "Utils/IOStream/ChildPipeIOStream.h"

#include "Utils/Time.hpp"

MAA_NS_BEGIN

ChildPipeIOStream::ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args)
    : child_( //
          exec, args, boost::process::std_out > pin_, boost::process::std_err > pin_, boost::process::std_in < pout_
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

    pout_ << data << std::endl;
    return true;
}

std::string ChildPipeIOStream::read(duration_t timeout)
{
    return read_some(std::numeric_limits<size_t>::max(), timeout);
}

std::string ChildPipeIOStream::read_some(size_t count, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();
    std::string result;

    while (child_.running() && result.size() < count && duration_since(start_time) < timeout) {
        auto read_size = std::min(kBufferSize, count - result.size());
        auto read_num = pin_.readsome(buffer_.get(), read_size);
        result.append(buffer_.get(), read_num);
    }

    return result;
}

std::string ChildPipeIOStream::read_until(std::string_view delimiter, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();

    std::string result;

    while (!result.ends_with(delimiter)) {
        auto sub_timeout = timeout - duration_since<duration_t>(start_time);
        if (sub_timeout < duration_t::zero()) {
            break;
        }

        auto sub_str = read_some(1, sub_timeout);
        result.append(std::move(sub_str));
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
