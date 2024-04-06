#include "Utils/IOStream/ChildPipeIOStream.h"

#include "Utils/Codec.h"
#include "Utils/Logger.h"

MAA_NS_BEGIN

#ifdef _WIN32
std::vector<std::wstring> conv_args(const std::vector<std::string>& args)
{
    std::vector<std::wstring> wargs;
    for (const auto& arg : args) {
        wargs.emplace_back(to_u16(arg));
    }
    return wargs;
}
#else
std::vector<std::string> conv_args(const std::vector<std::string>& args)
{
    return args;
}
#endif

ChildPipeIOStream::ChildPipeIOStream(
    const std::filesystem::path& exec,
    const std::vector<std::string>& args)
    : ChildPipeIOStream(exec, conv_args(args), false)
{
}

#ifdef _WIN32
ChildPipeIOStream::ChildPipeIOStream(
    const std::filesystem::path& exec,
    const std::vector<std::wstring>& wargs)
    : ChildPipeIOStream(exec, wargs, false)
{
}
#endif

ChildPipeIOStream::ChildPipeIOStream(
    const std::filesystem::path& exec,
    const std::vector<os_string>& args,
    bool)
    : exec_(exec)
    , args_(args)
    , child_(
          exec_,
          args_,
          boost::process::std_out > pin_,
          boost::process::std_err > boost::process::null,
          boost::process::std_in < pout_
#ifdef _WIN32
          ,
          boost::process::windows::create_no_window
#endif
      )
{
    LogTrace << VAR(exec_) << VAR(args_) << VAR(child_.id());
}

ChildPipeIOStream::~ChildPipeIOStream()
{
    release();
}

bool ChildPipeIOStream::write(std::string_view data)
{
    if (!pout_.good()) {
        LogError << "pout is not good" << VAR(exec_) << VAR(args_) << VAR(child_.id());
        return false;
    }

    pout_ << data << std::endl;
    return true;
}

bool ChildPipeIOStream::release()
{
    auto start_time = std::chrono::steady_clock::now();
    using namespace std::chrono_literals;
    while (child_.running() && duration_since(start_time) < 100ms) {
        std::this_thread::yield();
    }

    if (child_.running()) {
        child_.terminate();
    }
    else {
        child_.wait();
    }

    int code = child_.exit_code();

    if (code != 0) {
        LogWarn << "child exit with" << code << VAR(exec_) << VAR(args_) << VAR(child_.id());
        return false;
    }

    return true;
}

bool ChildPipeIOStream::is_open() const
{
    return !pin_.eof();
}

std::string ChildPipeIOStream::read_once(size_t max_count)
{
    constexpr size_t kBufferSize = 128 * 1024;

    if (!buffer_) {
        buffer_ = std::make_unique<char[]>(kBufferSize);
    }

    size_t count = std::min(kBufferSize, max_count);
    auto read = pin_.read(buffer_.get(), count).gcount();
    return std::string(buffer_.get(), read);
}

MAA_NS_END