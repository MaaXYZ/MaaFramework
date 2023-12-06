#include "Utils/IOStream/ChildPipeIOStream.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

ChildPipeIOStream::ChildPipeIOStream(const std::filesystem::path& exec, const std::vector<std::string>& args)
    : child_( //
          exec, args, boost::process::std_out > pin_, boost::process::std_err > pin_, boost::process::std_in < pout_
#ifdef _WIN32
          ,
          boost::process::windows::create_no_window
#endif
      )
{}

ChildPipeIOStream::~ChildPipeIOStream()
{
    release();
}

bool ChildPipeIOStream::write(std::string_view data)
{
    if (is_open()) {
        LogError << "not opened";
        return false;
    }

    pout_ << data << std::endl;
    return true;
}

bool ChildPipeIOStream::release()
{
    if (child_.running()) {
        child_.terminate();
    }
    else {
        child_.wait();
    }

    int code = child_.exit_code();

    if (code != 0) {
        LogError << "child exit with" << code;
        return false;
    }

    return true;
}

bool ChildPipeIOStream::is_open() const
{
    return pin_.is_open();
}

std::string ChildPipeIOStream::read_once(size_t max_count)
{
    constexpr size_t kBufferSize = 128 * 1024;

    if (!buffer_) {
        buffer_ = std::make_unique<char[]>(kBufferSize);
    }

    auto read_size = std::min(kBufferSize, max_count);
    auto read_num = pin_.readsome(buffer_.get(), read_size);
    return std::string(buffer_.get(), read_num);
}

MAA_NS_END
