#include "BoostIO.h"

#include <chrono>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

BoostIO::BoostIO() : ios_(new boost::asio::io_context), server_sock_(*ios_)
{
    support_socket_ = true;
}

BoostIO::~BoostIO() {}

int BoostIO::call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                          std::string& sock_data, int64_t timeout)
{
    using namespace std::chrono;

    auto start_time = std::chrono::steady_clock::now();

    single_page_buffer<char> pipe_buffer;
    single_page_buffer<char> sock_buffer;

    auto check_timeout = [&](const auto& start_time) -> bool {
        return timeout && timeout < duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    };

    // TODO: 想办法直接把cmd的后面塞进args
    std::vector<std::string> rcmd(cmd.begin() + 1, cmd.end());

    boost::process::ipstream pout;

    boost::process::child proc(boost::process::search_path(cmd[0]), boost::process::args(rcmd),
                               boost::process::std_out > pout);

    if (recv_by_socket) {
        auto socket = server_sock_.accept();
        if (!socket.is_open()) {
            return -1;
        }
        boost::system::error_code error;
        do {
            auto read_num = socket.read_some(boost::asio::mutable_buffer(pipe_buffer.get(), pipe_buffer.size()), error);
            while (error != boost::asio::error::eof && read_num > 0) {
                sock_data.insert(sock_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                read_num = socket.read_some(boost::asio::mutable_buffer(pipe_buffer.get(), pipe_buffer.size()), error);
            }
        } while (proc.running() && socket.is_open() && !check_timeout(start_time));
        proc.wait();
    }
    else {
        do {
            pout.read(pipe_buffer.get(), pipe_buffer.size());
            auto read_num = pout.gcount();
            while (read_num > 0) {
                pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                pout.read(pipe_buffer.get(), pipe_buffer.size());
                read_num = pout.gcount();
            }
        } while (proc.running() && !check_timeout(start_time));
        proc.wait();
    }

    if (proc.running()) {
        proc.terminate();
    }

    return proc.exit_code();
}

std::optional<unsigned short> BoostIO::create_socket(const std::string& local_address)
{
    using namespace boost::asio::ip;

    tcp::endpoint endPoint(tcp::endpoint(address::from_string(local_address), 0));

    server_sock_.open(endPoint.protocol());
    server_sock_.set_option(tcp::acceptor::reuse_address(true));
    server_sock_.bind(endPoint);

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

    tcp::socket socket(*ios_);

    socket.connect(tcp::endpoint(address::from_string(target), port));

    if (!socket.is_open()) {
        return nullptr;
    }

    return std::make_shared<IOHandlerBoostSocket>(ios_, std::move(socket));
}

std::shared_ptr<IOHandler> BoostIO::interactive_shell(const std::vector<std::string>& cmd)
{
    // TODO: 想办法直接把cmd的后面塞进args
    std::vector<std::string> rcmd(cmd.begin() + 1, cmd.end());

    std::shared_ptr<boost::process::opstream> pin(new boost::process::opstream);
    std::shared_ptr<boost::process::ipstream> pout(new boost::process::ipstream);
    std::shared_ptr<boost::process::child> proc(
        new boost::process::child(boost::process::search_path(cmd[0]), boost::process::args(rcmd),
                                  boost::process::std_in<*pin, boost::process::std_out> * pout));

    return std::make_shared<IOHandlerBoostStream>(pout, pin, proc);
}

IOHandlerBoostSocket::~IOHandlerBoostSocket()
{
    sock_.close();
}

bool IOHandlerBoostSocket::write(std::string_view data)
{
    if (!sock_.is_open()) {
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

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];

    std::string result;

    boost::system::error_code error;
    while (expect > result.size() && sock_.is_open() && check_timeout(start_time)) {
        auto maxi = std::min(bufferSize, expect - result.size());
        auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer, maxi), error);
        while (error != boost::asio::error::eof && read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            maxi = std::min(bufferSize, expect - result.size());
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

    in_->write(data.data(), data.size());
    return true;
}

std::string IOHandlerBoostStream::read(unsigned timeout_sec)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];

    std::string result;

    while (check_timeout(start_time)) {
        out_->read(buffer, bufferSize);
        auto read_num = out_->gcount();
        while (read_num > 0) {
            result.insert(result.end(), buffer, buffer + read_num);
            out_->read(buffer, bufferSize);
            read_num = out_->gcount();
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
