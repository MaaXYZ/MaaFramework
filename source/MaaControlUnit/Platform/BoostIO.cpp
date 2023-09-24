#include "BoostIO.h"

#include <chrono>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

#ifdef _WIN32
#define BOOST_CREATE_NO_WINDOW , boost::process::windows::create_no_window
#else
#define BOOST_CREATE_NO_WINDOW
#endif

MAA_CTRL_UNIT_NS_BEGIN

BoostIO::BoostIO() : ios_(std::make_shared<boost::asio::io_context>()), server_sock_(*ios_)
{
    support_socket_ = true;
}

BoostIO::~BoostIO() {}

int BoostIO::call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                          std::string& sock_data, int64_t timeout)
{
    using namespace std::chrono;

    if (cmd.empty()) {
        LogError << "cmd is empty";
        return -1;
    }

    auto exec = boost::process::search_path(cmd[0]);
    if (!std::filesystem::exists(exec)) {
        LogError << "path not exists" << VAR(exec) << VAR(cmd[0]);
        return -1;
    }
    // TODO: 想办法直接把cmd的后面塞进args
    std::vector<std::string> rcmd(cmd.begin() + 1, cmd.end());

    boost::process::ipstream pout;
    boost::process::child proc(exec, boost::process::args(rcmd),
                               boost::process::std_in<boost::process::null, boost::process::std_out> pout,
                               boost::process::std_err > boost::process::null BOOST_CREATE_NO_WINDOW);

    const auto start_time = std::chrono::steady_clock::now();
    auto terminate = [&]() -> bool {
        return !proc.running() || (timeout && timeout < duration_since(start_time).count());
    };

    if (recv_by_socket) {
        read_sock_data(sock_data, terminate);
    }
    else {
        read_pipe_data(pout, pipe_data, terminate);
    }

    while (!terminate()) {
        std::this_thread::yield();
    }

    if (proc.running()) {
        LogWarn << "terminate" << VAR(exec);
        proc.terminate();
    }
    else {
        proc.wait();
    }

    return proc.exit_code();
}

std::optional<unsigned short> BoostIO::create_socket(const std::string& local_address)
{
    using namespace boost::asio::ip;

    tcp::endpoint endpoint(tcp::endpoint(address::from_string(local_address), 0));

    server_sock_.open(endpoint.protocol());
    server_sock_.set_option(tcp::acceptor::reuse_address(true));
    server_sock_.bind(endpoint);

    server_sock_.listen();

    tcp::endpoint ep = server_sock_.local_endpoint();
    return ep.port();
}

void BoostIO::close_socket() noexcept
{
    server_sock_.close();
}

std::shared_ptr<IOHandler> BoostIO::tcp(const std::string& target, unsigned short port)
{
    using namespace boost::asio::ip;

    if (!ios_) {
        LogError << "ios_ is nullptr";
        return nullptr;
    }

    tcp::socket socket(*ios_);

    socket.connect(tcp::endpoint(address::from_string(target), port));

    if (!socket.is_open()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<IOHandlerBoostSocket>(ios_, std::move(socket));
}

std::shared_ptr<IOHandler> BoostIO::interactive_shell(const std::vector<std::string>& cmd, bool want_stderr)
{
    // TODO: 想办法直接把cmd的后面塞进args
    std::vector<std::string> rcmd(cmd.begin() + 1, cmd.end());

    std::shared_ptr<boost::process::opstream> pin(new boost::process::opstream);
    std::shared_ptr<boost::process::ipstream> pout(new boost::process::ipstream);

    std::shared_ptr<boost::process::child> proc(
        want_stderr ? new boost::process::child(boost::process::search_path(cmd[0]), boost::process::args(rcmd),
                                                boost::process::std_in<*pin, boost::process::std_err> *
                                                    pout BOOST_CREATE_NO_WINDOW)
                    : new boost::process::child(boost::process::search_path(cmd[0]), boost::process::args(rcmd),
                                                boost::process::std_in<*pin, boost::process::std_out> *
                                                    pout BOOST_CREATE_NO_WINDOW));

    return std::make_shared<IOHandlerBoostStream>(pout, pin, proc);
}

void BoostIO::read_sock_data(std::string& data, std::function<bool(void)> terminate)
{
    constexpr size_t kBufferSize = 128 * 1024;
    auto buffer = std::make_unique<char[]>(kBufferSize);

    auto socket = server_sock_.accept();
    if (!socket.is_open()) {
        LogError << "socket is not opened";
        return;
    }
    boost::system::error_code error;
    do {
        memset(buffer.get(), 0, kBufferSize);
        auto read_num = socket.read_some(boost::asio::mutable_buffer(buffer.get(), kBufferSize), error);
        while (error != boost::asio::error::eof && read_num > 0) {
            data.insert(data.end(), buffer.get(), buffer.get() + read_num);
            read_num = socket.read_some(boost::asio::mutable_buffer(buffer.get(), kBufferSize), error);
        }
    } while (socket.is_open() && !terminate());
}

void BoostIO::read_pipe_data(boost::process::ipstream& pout, std::string& data, std::function<bool(void)> terminate)
{
    constexpr size_t kBufferSize = 128 * 1024;
    auto buffer = std::make_unique<char[]>(kBufferSize);

    do {
        if (pout.rdbuf()->in_avail() == 0) {
            char ch;
            pout.read(&ch, 1);
            if (pout.gcount() != 1) {
                break;
            }
            data.push_back(ch);
        }

        memset(buffer.get(), 0, kBufferSize);
        auto read_num = pout.readsome(buffer.get(), kBufferSize);
        while (read_num > 0) {
            data.insert(data.end(), buffer.get(), buffer.get() + read_num);
            read_num = pout.readsome(buffer.get(), kBufferSize);
        }
    } while (!terminate());
}

IOHandlerBoostSocket::~IOHandlerBoostSocket()
{
    sock_.close();
}

bool IOHandlerBoostSocket::write(std::string_view data)
{
    if (!sock_.is_open()) {
        LogError << "socket is not opened";
        return false;
    }
    sock_.write_some(boost::asio::buffer(data));
    return true;
}

std::string IOHandlerBoostSocket::read(unsigned timeout_sec)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];

    std::string result;

    boost::system::error_code error;
    while (check_timeout(start_time) && sock_.is_open()) {
        auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer, bufferSize), error);
        while (error != boost::asio::error::eof && read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            read_num = sock_.read_some(boost::asio::mutable_buffer(buffer, bufferSize), error);
        }
        break;
    }

    return result;
}

std::string IOHandlerBoostSocket::read(unsigned timeout_sec, size_t expect)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    constexpr size_t kBufferSize = 4096;
    char buffer[kBufferSize] = { 0 };

    std::string result;

    boost::system::error_code error;
    while (expect > result.size() && sock_.is_open() && check_timeout(start_time)) {
        auto maxi = std::min(kBufferSize, expect - result.size());
        auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer, maxi), error);
        while (error != boost::asio::error::eof && read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            maxi = std::min(kBufferSize, expect - result.size());
            read_num = sock_.read_some(boost::asio::mutable_buffer(buffer, maxi), error);
        }
    }

    return result;
}

IOHandlerBoostStream::~IOHandlerBoostStream()
{
    if (proc_->running()) {
        proc_->terminate();
    }
}

bool IOHandlerBoostStream::write(std::string_view data)
{
    if (!proc_->running()) {
        return false;
    }

    // std::cerr << "write " << std::string(data.data(), data.data() + data.size()) << std::endl;
    in_->write(data.data(), data.size());
    in_->flush();
    return true;
}

std::string IOHandlerBoostStream::read(unsigned timeout_sec)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    constexpr size_t kBufferSize = 4096;
    char buffer[kBufferSize] = { 0 };

    std::string result;

    // dirty hack
    if (out_->rdbuf()->in_avail() == 0) {
        out_->read(buffer, 1);
        result.push_back(buffer[0]);
    }

    while (check_timeout(start_time)) {
        auto read_num = out_->readsome(buffer, kBufferSize);
        while (read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            read_num = out_->readsome(buffer, kBufferSize);
        }
        break;
    }

    return result;
}

std::string IOHandlerBoostStream::read(unsigned timeout_sec, size_t expect)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];

    std::string result;

    while (expect > result.size() && check_timeout(start_time)) {
        auto maxi = std::min(bufferSize, expect - result.size());
        auto read_num = out_->readsome(buffer, maxi);
        while (read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            maxi = std::min(bufferSize, expect - result.size());
            read_num = out_->readsome(buffer, maxi);
        }
    }

    return result;
}

MAA_CTRL_UNIT_NS_END
