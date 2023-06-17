#ifndef _WIN32

#include "PosixIO.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/socket.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstring> // for strerror

#include "Utils/Logger.hpp"
#include "Utils/Platform/Platform.h"

MAA_CTRL_NS_BEGIN

PosixIO::PosixIO()
{
    support_socket_ = true;

    int pipe_in_ret = ::pipe(pipe_in_);
    int pipe_out_ret = ::pipe(pipe_out_);
    ::fcntl(pipe_out_[PIPE_READ], F_SETFL, O_NONBLOCK);

    if (pipe_in_ret < 0 || pipe_out_ret < 0) {
        auto err = strerror(errno);
        LogError << "controller pipe created failed" << VAR(err);
    }
}
PosixIO::~PosixIO()
{
    if (server_sock_ >= 0) {
        ::close(server_sock_);
        server_sock_ = -1;
    }

    ::close(pipe_in_[PIPE_READ]);
    ::close(pipe_in_[PIPE_WRITE]);
    ::close(pipe_out_[PIPE_READ]);
    ::close(pipe_out_[PIPE_WRITE]);
}

int PosixIO::call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                          std::string& sock_data, int64_t timeout)
{
    using namespace std::chrono;

    auto start_time = std::chrono::steady_clock::now();

    MAA_PLATFORM_NS::single_page_buffer<char> pipe_buffer;
    MAA_PLATFORM_NS::single_page_buffer<char> sock_buffer;

    auto check_timeout = [&](const auto& start_time) -> bool {
        return timeout && timeout < duration_cast<milliseconds>(steady_clock::now() - start_time).count();
    };

    int exit_ret = 0;
    child_ = ::fork();
    if (child_ == 0) {
        // child process

        ::dup2(pipe_in_[PIPE_READ], STDIN_FILENO);
        ::dup2(pipe_out_[PIPE_WRITE], STDOUT_FILENO);
        ::dup2(pipe_out_[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        // close(pipe_in_[PIPE_READ]);
        // close(pipe_in_[PIPE_WRITE]);
        // close(pipe_out_[PIPE_READ]);
        // close(pipe_out_[PIPE_WRITE]);

        char** argv = new char*[cmd.size() + 1];
        for (size_t i = 0; i < cmd.size(); i++) {
            argv[i] = const_cast<char*>(cmd[i].c_str());
        }
        argv[cmd.size()] = NULL;
        exit_ret = execvp(cmd[0].c_str(), argv);
        LogError << "fork failed" << strerror(errno);
        ::exit(exit_ret);
    }
    else if (child_ > 0) {
        // parent process
        if (recv_by_socket) {
            sockaddr addr {};
            socklen_t len = sizeof(addr);
            sock_buffer = MAA_PLATFORM_NS::single_page_buffer<char>();

            int client_socket = ::accept(server_sock_, &addr, &len);
            if (client_socket < 0) {
                LogError << "accept failed:" << strerror(errno);
                ::kill(child_, SIGKILL);
                ::waitpid(child_, &exit_ret, 0);
                return -1;
            }

            ssize_t read_num = ::read(client_socket, sock_buffer.get(), sock_buffer.size());
            while (read_num > 0) {
                sock_data.insert(sock_data.end(), sock_buffer.get(), sock_buffer.get() + read_num);
                read_num = ::read(client_socket, sock_buffer.get(), sock_buffer.size());
            }
            ::shutdown(client_socket, SHUT_RDWR);
            ::close(client_socket);
        }
        else {
            do {
                ssize_t read_num = ::read(pipe_out_[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());
                while (read_num > 0) {
                    pipe_data.insert(pipe_data.end(), pipe_buffer.get(), pipe_buffer.get() + read_num);
                    read_num = ::read(pipe_out_[PIPE_READ], pipe_buffer.get(), pipe_buffer.size());
                }
            } while (::waitpid(child_, &exit_ret, WNOHANG) == 0 && !check_timeout(start_time));
        }
        ::waitpid(child_, &exit_ret, 0); // if ::waitpid(child_, &exit_ret, WNOHANG) == 0, repeat it will cause
        // ECHILD, so not check the return value
    }
    else {
        // failed to create child process
        LogError << "Call `" << VAR(cmd) << "` create process failed, child:" << VAR(child_);
        return -1;
    }

    return exit_ret;
}

std::optional<unsigned short> PosixIO::create_socket(const std::string& local_address)
{
    server_sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_ < 0) {
        return std::nullopt;
    }

    server_sock_addr_.sin_family = AF_INET;
    server_sock_addr_.sin_addr.s_addr = INADDR_ANY;

    bool server_start = false;
    uint16_t port_result = 0;

    server_sock_addr_.sin_port = htons(0);
    int bind_ret = ::bind(server_sock_, reinterpret_cast<sockaddr*>(&server_sock_addr_), sizeof(::sockaddr_in));
    socklen_t addrlen = sizeof(server_sock_addr_);
    int getname_ret = ::getsockname(server_sock_, reinterpret_cast<sockaddr*>(&server_sock_addr_), &addrlen);
    int listen_ret = ::listen(server_sock_, 3);
    struct timeval timeout = { 6, 0 };
    int timeout_ret = ::setsockopt(server_sock_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
    server_start = bind_ret == 0 && getname_ret == 0 && listen_ret == 0 && timeout_ret == 0;

    if (!server_start) {
        LogInfo << "not supports socket";
        return std::nullopt;
    }

    port_result = ntohs(server_sock_addr_.sin_port);

    LogInfo << "command server start" << VAR(local_address) << VAR(port_result);
    return port_result;
}

void PosixIO::close_socket() noexcept
{
    if (server_sock_ >= 0) {
        ::close(server_sock_);
        server_sock_ = -1;
    }
}

std::shared_ptr<IOHandler> PosixIO::tcp(const std::string& target, unsigned short port)
{
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return nullptr;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(target.c_str());
    addr.sin_port = htons(port);
    int ret = ::connect(sock, (sockaddr*)&addr, sizeof(addr));

    if (ret < 0) {
        return nullptr;
    }

    return std::make_shared<IOHandlerPosix>(sock, sock, 0);
}

std::shared_ptr<IOHandler> PosixIO::interactive_shell(const std::vector<std::string>& cmd)
{
    int pipe_to_child[2];
    int pipe_from_child[2];

    if (::pipe(pipe_to_child)) return nullptr;
    if (::pipe(pipe_from_child)) {
        ::close(pipe_to_child[0]);
        ::close(pipe_to_child[1]);
        return nullptr;
    }

    ::pid_t pid = fork();
    if (pid < 0) {
        ::close(pipe_to_child[0]);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        ::close(pipe_from_child[1]);
        LogError << "fork failed:" << strerror(errno);
        return nullptr;
    }
    if (pid == 0) {
        // child process
        if (::dup2(pipe_to_child[0], STDIN_FILENO) < 0 || ::close(pipe_to_child[1]) < 0 ||
            ::close(pipe_from_child[0]) < 0 || ::dup2(pipe_from_child[1], STDOUT_FILENO) < 0 ||
            ::dup2(pipe_from_child[1], STDERR_FILENO) < 0) {
            LogError << "pipe failed" << strerror(errno);
            ::exit(-1);
        }

        // set stdin of child to blocking
        if (int val = ::fcntl(STDIN_FILENO, F_GETFL); val != -1 && (val & O_NONBLOCK)) {
            val &= ~O_NONBLOCK;
            ::fcntl(STDIN_FILENO, F_SETFL, val);
        }

#ifndef __APPLE__
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

        char** argv = new char*[cmd.size() + 1];
        for (size_t i = 0; i < cmd.size(); i++) {
            argv[i] = const_cast<char*>(cmd[i].c_str());
        }
        argv[cmd.size()] = NULL;
        auto exit_ret = execvp(cmd[0].c_str(), argv);
        LogError << "fork failed" << strerror(errno);
        ::exit(exit_ret);
    }

    if (::close(pipe_to_child[0]) < 0 || ::close(pipe_from_child[1]) < 0) {
        ::kill(pid, SIGTERM);
        ::waitpid(pid, nullptr, 0);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        return nullptr;
    }

    // set stdout to non blocking
    if (int val = ::fcntl(pipe_from_child[0], F_GETFL); val != -1) {
        val |= O_NONBLOCK;
        ::fcntl(pipe_from_child[0], F_SETFL, val);
    }
    else {
        ::kill(pid, SIGTERM);
        ::waitpid(pid, nullptr, 0);
        ::close(pipe_to_child[1]);
        ::close(pipe_from_child[0]);
        return nullptr;
    }

    return std::make_shared<IOHandlerPosix>(pipe_from_child[0], pipe_to_child[1], pid);
}

IOHandlerPosix::~IOHandlerPosix()
{
    if (write_fd_ != -1) ::close(write_fd_);
    if (read_fd_ != -1 && read_fd_ != write_fd_) ::close(read_fd_);
    if (process_ > 0) ::kill(process_, SIGTERM);
}

bool IOHandlerPosix::write(std::string_view data)
{
    if (process_ < 0 || write_fd_ < 0) return false;
    if (::write(write_fd_, data.data(), data.length()) >= 0) return true;
    LogError << "Failed to write to IOHandlerPosix, err" << strerror(errno);
    return false;
}

std::string IOHandlerPosix::read(unsigned timeout_sec)
{
    if (process_ < 0 || read_fd_ < 0) return {};
    std::string ret_str;
    constexpr int PipeReadBuffSize = 4096ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        char buf_from_child[PipeReadBuffSize];

        if (!check_timeout(start_time)) {
            break;
        }

        ssize_t ret_read = ::read(read_fd_, buf_from_child, PipeReadBuffSize);
        if (ret_read > 0) {
            ret_str.insert(ret_str.end(), buf_from_child, buf_from_child + ret_read);
        }
        else {
            break;
        }
    }
    return ret_str;
}

std::string IOHandlerPosix::read(unsigned timeout_sec, size_t expect)
{
    if (process_ < 0 || read_fd_ < 0) return {};
    std::string ret_str;
    constexpr size_t PipeReadBuffSize = 4096ULL;

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < timeout_sec * 1s;
    };

    auto start_time = std::chrono::steady_clock::now();

    while (ret_str.size() < expect) {
        char buf_from_child[PipeReadBuffSize];

        if (!check_timeout(start_time)) {
            break;
        }

        ssize_t ret_read = ::read(read_fd_, buf_from_child, std::min(PipeReadBuffSize, expect - ret_str.size()));
        if (ret_read > 0) {
            ret_str.insert(ret_str.end(), buf_from_child, buf_from_child + ret_read);
        }
        else {
            // break;
        }
    }
    return ret_str;
}

MAA_CTRL_NS_END

#endif
